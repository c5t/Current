/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>
          (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include "sherlock.h"
#include "kv_storage.h"

#include <string>
#include <atomic>
#include <thread>

#include "../Bricks/strings/util.h"
#include "../Bricks/cerealize/cerealize.h"
#include "../Bricks/net/api/api.h"
#include "../Bricks/time/chrono.h"

#include "../Bricks/dflags/dflags.h"
#include "../Bricks/3party/gtest/gtest-main-with-dflags.h"

DEFINE_int32(sherlock_http_test_port, 8090, "Local port to use for Sherlock unit test.");

using std::string;
using std::atomic_size_t;
using std::thread;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

using bricks::strings::Printf;
using bricks::strings::ToString;

using bricks::time::EPOCH_MILLISECONDS;
using bricks::time::MILLISECONDS_INTERVAL;

// The records we work with.
// TODO(dkorolev): Support and test polymorphic types.
struct Record {
  int x_;
  Record(int x = 0) : x_(x) {}
  template <typename A>
  void serialize(A& ar) {
    ar(cereal::make_nvp("x", x_));
  }
};

struct RecordWithTimestamp {
  // TODO(dkorolev): Make the `EPOCH_MILLISECONDS` type serializable.
  std::string s_;
  uint64_t timestamp_;
  RecordWithTimestamp(std::string s = "", EPOCH_MILLISECONDS timestamp = EPOCH_MILLISECONDS(0))
      : s_(s), timestamp_(static_cast<uint64_t>(timestamp)) {}
  template <typename A>
  void serialize(A& ar) {
    ar(cereal::make_nvp("s", s_), cereal::make_nvp("t", timestamp_));
  }

  EPOCH_MILLISECONDS ExtractTimestamp() const { return static_cast<EPOCH_MILLISECONDS>(timestamp_); }
};

// Struct `Data` should be outside struct `Processor`, since the latter is `std::move`-d away in some tests.
struct Data {
  atomic_size_t seen_;
  string results_;
  Data() : seen_(0u) {}
};

// Struct `Processor` handles the entries that tests subscribe to.
struct Processor {
  Data& data_;
  size_t max_to_process_ = static_cast<size_t>(-1);

  Processor() = delete;
  explicit Processor(Data& data) : data_(data) {}

  Processor& SetMax(size_t cap) {
    max_to_process_ = cap;
    return *this;
  }

  inline bool Entry(const Record& entry, size_t index, size_t total) {
    static_cast<void>(index);
    static_cast<void>(total);
    if (data_.seen_) {
      data_.results_ += ",";
    }
    data_.results_ += ToString(entry.x_);
    ++data_.seen_;
    return data_.seen_ < max_to_process_;
  }

  inline void Terminate() {
    if (data_.seen_) {
      data_.results_ += ",";
    }
    data_.results_ += "DONE";
  }
};

TEST(Sherlock, SubscribeAndProcessThreeEntries) {
  auto foo_stream = sherlock::Stream<Record>("foo");
  foo_stream.Publish(1);
  foo_stream.Publish(2);
  foo_stream.Publish(3);
  Data d;
  Processor p(d);
  foo_stream.Subscribe(p.SetMax(3u))
      .Join();  // `.Join()` makes this a blocking call, waiting for three entries.
  EXPECT_EQ(d.seen_, 3u);
  EXPECT_EQ("1,2,3", d.results_);
}

TEST(Sherlock, SubscribeAndProcessThreeEntriesByUniquePtr) {
  auto bar_stream = sherlock::Stream<Record>("bar");
  bar_stream.Publish(4);
  bar_stream.Publish(5);
  bar_stream.Publish(6);
  Data d;
  std::unique_ptr<Processor> p(new Processor(d));
  p->SetMax(3u);
  bar_stream.Subscribe(std::move(p))
      .Join();  // `.Join()` makes this a blocking call, waiting for three entries.
  EXPECT_EQ(d.seen_, 3u);
  EXPECT_EQ("4,5,6", d.results_);
}

TEST(Sherlock, SubscribeHandleGoesOutOfScopeBeforeAnyProcessing) {
  auto baz_stream = sherlock::Stream<Record>("baz");
  thread delayed_publish_thread([&baz_stream]() {
    sleep_for(milliseconds(10));
    baz_stream.Publish(7);
    baz_stream.Publish(8);
    baz_stream.Publish(9);
  });
  Data d;
  Processor p(d);
  baz_stream.Subscribe(p);
  EXPECT_EQ(d.seen_, 0u);
  EXPECT_EQ("DONE", d.results_);
  delayed_publish_thread.join();
}

TEST(Sherlock, SubscribeProcessedThreeEntriesBecauseWeWaitInTheScope) {
  auto meh_stream = sherlock::Stream<Record>("meh");
  meh_stream.Publish(10);
  meh_stream.Publish(11);
  meh_stream.Publish(12);
  Data d;
  Processor p(d);
  {
    auto scope = meh_stream.Subscribe(p);
    {
      auto scope2 = std::move(scope);
      {
        auto scope3 = std::move(scope2);
        while (d.seen_ < 3u) {
          ;  // Spin lock.
        }
      }
    }
  }
  EXPECT_EQ(d.seen_, 3u);
  EXPECT_EQ("10,11,12,DONE", d.results_);
}

TEST(Sherlock, SubscribeToStreamViaHTTP) {
  // Publish four records.
  // { "s[0]", "s[1]", "s[2]", "s[3]" } 40, 30, 20 and 10 seconds ago respectively.
  auto exposed_stream = sherlock::Stream<RecordWithTimestamp>("exposed");
  const EPOCH_MILLISECONDS now = bricks::time::Now();
  exposed_stream.Emplace("s[0]", now - MILLISECONDS_INTERVAL(40000));
  exposed_stream.Emplace("s[1]", now - MILLISECONDS_INTERVAL(30000));
  exposed_stream.Emplace("s[2]", now - MILLISECONDS_INTERVAL(20000));
  exposed_stream.Emplace("s[3]", now - MILLISECONDS_INTERVAL(10000));

  // Collect them and store as strings.
  // Required since we don't mock time for this test, and therefore can't do exact match.
  struct RecordsCollector {
    atomic_size_t count_;
    std::vector<std::string>& data_;

    RecordsCollector() = delete;
    explicit RecordsCollector(std::vector<std::string>& data) : count_(0u), data_(data) {}

    inline bool Entry(const RecordWithTimestamp& entry, size_t index, size_t total) {
      static_cast<void>(index);
      static_cast<void>(total);
      data_.push_back(JSON(entry, "entry") + '\n');
      ++count_;
      return true;
    }
    inline void Terminate() {}
  };

  std::vector<std::string> s;
  RecordsCollector collector(s);
  {
    auto scope = exposed_stream.Subscribe(collector);
    while (collector.count_ < 4u) {
      ;  // Spin lock.
    }
  }
  EXPECT_EQ(s.size(), 4u);

  HTTP(FLAGS_sherlock_http_test_port).ResetAllHandlers();
  HTTP(FLAGS_sherlock_http_test_port).Register("/exposed", exposed_stream);

  // Test `?n=...`.
  EXPECT_EQ(s[3], HTTP(GET(Printf("http://localhost:%d/exposed?n=1", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(s[2] + s[3],
            HTTP(GET(Printf("http://localhost:%d/exposed?n=2", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(s[1] + s[2] + s[3],
            HTTP(GET(Printf("http://localhost:%d/exposed?n=3", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(s[0] + s[1] + s[2] + s[3],
            HTTP(GET(Printf("http://localhost:%d/exposed?n=4", FLAGS_sherlock_http_test_port))).body);
  // `?n={>4}` will block forever.

  // Test `?cap=...`.
  EXPECT_EQ(s[0] + s[1] + s[2] + s[3],
            HTTP(GET(Printf("http://localhost:%d/exposed?cap=4", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(s[0] + s[1],
            HTTP(GET(Printf("http://localhost:%d/exposed?cap=2", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(s[0], HTTP(GET(Printf("http://localhost:%d/exposed?cap=1", FLAGS_sherlock_http_test_port))).body);

  // Test `?recent=...`, have to use `?cap=...`.
  EXPECT_EQ(
      s[3],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=1&recent=15000", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(
      s[2],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=1&recent=25000", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(
      s[1],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=1&recent=35000", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(
      s[0],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=1&recent=45000", FLAGS_sherlock_http_test_port))).body);

  EXPECT_EQ(
      s[2] + s[3],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=2&recent=25000", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(
      s[1] + s[2],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=2&recent=35000", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(
      s[0] + s[1],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=2&recent=45000", FLAGS_sherlock_http_test_port))).body);

  EXPECT_EQ(
      s[1] + s[2] + s[3],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=3&recent=35000", FLAGS_sherlock_http_test_port))).body);
  EXPECT_EQ(
      s[0] + s[1] + s[2],
      HTTP(GET(Printf("http://localhost:%d/exposed?cap=3&recent=45000", FLAGS_sherlock_http_test_port))).body);

  // TODO(dkorolev): Add tests that add data while the chunked response is in progress.
  // TODO(dkorolev): Unregister the exposed endpoint and free its handler. It's hanging out there now...
  // TODO(dkorolev): Add tests that the endpoint is not unregistered until its last client is done. (?)
}

struct IntKey {
  int x;
  IntKey(int x = 0) : x(x) {}
  int operator()() const { return x; }

  template <typename A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(x));
  }

  bool operator==(const IntKey& rhs) const { return x == rhs.x; }
  struct HashFunction {
    size_t operator()(const IntKey& k) const { return static_cast<size_t>(k.x); }
  };
};

struct KeyValueEntry {
  IntKey key_;
  double value_;
  // Uncomment the line below to ensure that it doesn't compile.
  // constexpr static bool allow_nonthrowing_get = true;

  KeyValueEntry() = default;
  KeyValueEntry(const int key, const double value) : key_(key), value_(value) {}

  const IntKey& key() const { return key_; }
  void set_key(const IntKey& key) { key_ = key; }

  template <typename A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(key_), CEREAL_NVP(value_));
  }
};

struct KeyValueSubscriptionData {
  atomic_size_t seen_;
  string results_;
  KeyValueSubscriptionData() : seen_(0u) {}
};

struct KeyValueAggregateListener {
  KeyValueSubscriptionData& data_;
  size_t max_to_process_ = static_cast<size_t>(-1);

  KeyValueAggregateListener() = delete;
  explicit KeyValueAggregateListener(KeyValueSubscriptionData& data) : data_(data) {}

  KeyValueAggregateListener& SetMax(size_t cap) {
    max_to_process_ = cap;
    return *this;
  }

  bool Entry(const KeyValueEntry& entry, size_t index, size_t total) {
    static_cast<void>(index);
    static_cast<void>(total);
    if (data_.seen_) {
      data_.results_ += ",";
    }
    data_.results_ += Printf("%d=%.2lf", entry.key(), entry.value_);
    ++data_.seen_;
    return data_.seen_ < max_to_process_;
  }

  void Terminate() {
    if (data_.seen_) {
      data_.results_ += ",";
    }
    data_.results_ += "DONE";
  }
};

TEST(Sherlock, NonPolymorphicKeyValueStorage) {
  typedef sherlock::kvs::API<KeyValueEntry> KVS;
  KVS api("non_polymorphic_kvs");

  // Add the first key-value pair.
  // Use `UnsafeStream()`, since generally the only way to access the underlying stream is to make API calls.
  api.UnsafeStream().Emplace(2, 0.5);

  while (!api.CaughtUp()) {
    // Spin lock, for the purposes of this test.
    // Ensure that the data has reached the the processor that maintains the in-memory state of the API.
  }

  // Future expanded syntax.
  std::future<KeyValueEntry> f1 = api.AsyncGet(KVS::T_KEY(2));
  KeyValueEntry r1 = f1.get();
  EXPECT_EQ(2, r1.key()());
  EXPECT_EQ(0.5, r1.value_);

  // Future short syntax.
  EXPECT_EQ(0.5, api.AsyncGet(KVS::T_KEY(2)).get().value_);

  // Callback version.
  struct CallbackTest {
    explicit CallbackTest(int key, double value, bool expect_success = true)
        : key(key), value(value), expect_success(expect_success) {}

    void found(const KeyValueEntry& entry) const {
      called = true;
      EXPECT_TRUE(expect_success);
      EXPECT_EQ(key, entry.key()());
      EXPECT_EQ(value, entry.value_);
    }
    void not_found(const IntKey& key) const {
      called = true;
      EXPECT_FALSE(expect_success);
      EXPECT_EQ(this->key, key());
    }
    void added() const {
      called = true;
      EXPECT_TRUE(expect_success);
    }
    void already_exists() const {
      called = true;
      EXPECT_FALSE(expect_success);
    }

    const int key;
    const double value;
    const bool expect_success;
    mutable bool called = false;
  };

  const CallbackTest cbt1(2, 0.5);
  api.AsyncGet(KVS::T_KEY(2),
               std::bind(&CallbackTest::found, &cbt1, std::placeholders::_1),
               std::bind(&CallbackTest::not_found, &cbt1, std::placeholders::_1));
  while (!cbt1.called)
    ;

  // Add two more key-value pairs.
  api.UnsafeStream().Emplace(3, 0.33);
  api.UnsafeStream().Emplace(4, 0.25);

  while (api.EntriesSeen() < 3u) {
    // For the purposes of this test: Spin lock to ensure that the listener/MMQ consumer got the data published.
  }

  EXPECT_EQ(0.33, api.AsyncGet(KVS::T_KEY(3)).get().value_);
  EXPECT_EQ(0.25, api.Get(KVS::T_KEY(4)).value_);

  ASSERT_THROW(api.AsyncGet(KVS::T_KEY(5)).get(), KVS::T_KEY_NOT_FOUND_EXCEPTION);
  ASSERT_THROW(api.AsyncGet(KVS::T_KEY(5)).get(), sherlock::kvs::KeyNotFoundCoverException);
  ASSERT_THROW(api.Get(KVS::T_KEY(6)), KVS::T_KEY_NOT_FOUND_EXCEPTION);
  ASSERT_THROW(api.Get(KVS::T_KEY(6)), sherlock::kvs::KeyNotFoundCoverException);
  const CallbackTest cbt2(7, 0.0, false);
  api.AsyncGet(KVS::T_KEY(7),
               std::bind(&CallbackTest::found, &cbt2, std::placeholders::_1),
               std::bind(&CallbackTest::not_found, &cbt2, std::placeholders::_1));
  while (!cbt2.called)
    ;

  // Add three more key-value pairs, this time via the API.
  api.AsyncAdd(KeyValueEntry(5, 0.2)).wait();
  api.Add(KeyValueEntry(6, 0.17));
  const CallbackTest cbt3(7, 0.76);
  api.AsyncAdd(KVS::T_ENTRY(7, 0.76),
               std::bind(&CallbackTest::added, &cbt3),
               std::bind(&CallbackTest::already_exists, &cbt3));
  while (!cbt3.called)
    ;

  // Check that default policy doesn't allow overwriting on Add().
  ASSERT_THROW(api.AsyncAdd(KeyValueEntry(5, 1.1)).get(), KVS::T_KEY_ALREADY_EXISTS_EXCEPTION);
  ASSERT_THROW(api.AsyncAdd(KeyValueEntry(5, 1.1)).get(), sherlock::kvs::KeyAlreadyExistsCoverException);
  ASSERT_THROW(api.Add(KeyValueEntry(6, 0.28)), KVS::T_KEY_ALREADY_EXISTS_EXCEPTION);
  ASSERT_THROW(api.Add(KeyValueEntry(6, 0.28)), sherlock::kvs::KeyAlreadyExistsCoverException);
  const CallbackTest cbt4(7, 0.0, false);
  api.AsyncAdd(KVS::T_ENTRY(7, 0.0),
               std::bind(&CallbackTest::added, &cbt4),
               std::bind(&CallbackTest::already_exists, &cbt4));
  while (!cbt4.called)
    ;

  // Thanks to eventual consistency, we don't have to wait until the above calls fully propagate.
  // Even if the next two lines run before the entries are published into the stream,
  // the API will maintain the consistency of its own responses from its own in-memory state.
  EXPECT_EQ(0.20, api.AsyncGet(IntKey(5)).get().value_);
  EXPECT_EQ(0.17, api.Get(IntKey(6)).value_);

  ASSERT_THROW(api.AsyncGet(IntKey(8)).get(), KVS::T_KEY_NOT_FOUND_EXCEPTION);
  ASSERT_THROW(api.Get(IntKey(9)), KVS::T_KEY_NOT_FOUND_EXCEPTION);

  // Confirm that data updates have been pubished as stream entries as well.
  // This part is important since otherwise the API is no better than a wrapper over a hash map.
  KeyValueSubscriptionData data;
  KeyValueAggregateListener listener(data);
  listener.SetMax(6u);
  api.Subscribe(listener).Join();
  EXPECT_EQ(data.seen_, 6u);
  EXPECT_EQ("2=0.50,3=0.33,4=0.25,5=0.20,6=0.17,7=0.76", data.results_);
}
