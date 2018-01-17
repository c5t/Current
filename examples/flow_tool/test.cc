/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2018 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

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

#define CURRENT_MOCK_TIME

#include "flow_tool.h"

#include "../../bricks/dflags/dflags.h"
#include "../../bricks/file/file.h"

#include "../../3rdparty/gtest/gtest-main-with-dflags.h"

#ifndef CURRENT_WINDOWS
DEFINE_string(flow_tool_test_tmpdir, ".current", "Local path for the test to create temporary files in.");
#else
DEFINE_string(flow_tool_test_tmpdir, ".", "Local path for the test to create temporary files in.");
#endif

DEFINE_int32(flow_tool_test_port, PickPortForUnitTest(), "Local port to run the test against.");

TEST(FlowTool, Healthz) {
  current::time::ResetToZero();

  using flow_tool_t = flow_tool::FlowTool<flow_tool::db::FlowToolDB, StreamInMemoryStreamPersister>;

  flow_tool_t flow_tool(FLAGS_flow_tool_test_port, "");

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/up", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    EXPECT_TRUE(ParseJSON<flow_tool::api::Healthz>(response.body).up);
  }
}

TEST(FlowTool, RootNodeIsCreatedAutomatically) {
  current::time::ResetToZero();

  using flow_tool_t = flow_tool::FlowTool<flow_tool::db::FlowToolDB, StreamInMemoryStreamPersister>;
  using db_t = typename flow_tool_t::storage_t;

  flow_tool_t flow_tool(FLAGS_flow_tool_test_port, "");

  flow_tool.ImmutableDB()
      ->ReadOnlyTransaction([](ImmutableFields<db_t> fields) {
        EXPECT_EQ(1u, fields.node.Size());                                      // Must have one node.
        EXPECT_TRUE(Exists(fields.node[flow_tool::db::node_key_t::RootNode]));  // The root one.
        EXPECT_EQ(0u, fields.blob.Size());                                      // Must have no blobs yet.
      })
      .Wait();
}

TEST(FlowTool, GetsAnEmptyDirectory) {
  current::time::ResetToZero();

  using flow_tool_t = flow_tool::FlowTool<flow_tool::db::FlowToolDB, StreamInMemoryStreamPersister>;

  flow_tool_t flow_tool(FLAGS_flow_tool_test_port, "");

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::DirResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto dir_response = Value<flow_tool::api::success::DirResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://", dir_response.url);
    EXPECT_EQ("/", dir_response.path);
    EXPECT_EQ(0u, dir_response.dir.size());
  }
}

TEST(FlowTool, Returns404OnNonExistentFile) {
  current::time::ResetToZero();

  using flow_tool_t = flow_tool::FlowTool<flow_tool::db::FlowToolDB, StreamInMemoryStreamPersister>;
  flow_tool_t flow_tool(FLAGS_flow_tool_test_port, "");

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/test.txt", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(404, static_cast<int>(response.code)) << response.body;
    EXPECT_EQ("ErrorPathNotFound", ParseJSON<flow_tool::api::error::ErrorBase>(response.body).error);
  }
}

TEST(FlowTool, CorrectlyReturnsAManuallyInjectedFile) {
  current::time::ResetToZero();

  using flow_tool_t = flow_tool::FlowTool<flow_tool::db::FlowToolDB, StreamInMemoryStreamPersister>;
  using db_t = typename flow_tool_t::storage_t;

  flow_tool_t flow_tool(FLAGS_flow_tool_test_port, "");

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::DirResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto dir_response = Value<flow_tool::api::success::DirResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://", dir_response.url);
    EXPECT_EQ("/", dir_response.path);
    EXPECT_EQ(0u, dir_response.dir.size());
  }

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/test.txt", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(404, static_cast<int>(response.code)) << response.body;
    EXPECT_EQ("ErrorPathNotFound", ParseJSON<flow_tool::api::error::ErrorBase>(response.body).error);
    const auto error = ParseJSON<flow_tool::api::error::ErrorPathNotFound>(response.body);
    EXPECT_EQ("ErrorPathNotFound", error.error);
    EXPECT_EQ("/test.txt", error.path);
    EXPECT_EQ("test.txt", error.not_found_component);
    EXPECT_EQ(0u, error.not_found_component_zero_based_index);
  }

  flow_tool.MutableDB()
      ->ReadWriteTransaction([](MutableFields<db_t> fields) {
        {
          flow_tool::db::Blob file_blob;
          file_blob.key = static_cast<flow_tool::db::blob_key_t>(201ull);
          file_blob.body = "Hello, World!\n";
          fields.blob.Add(file_blob);
        }
        {
          flow_tool::db::Node file_node;
          file_node.key = static_cast<flow_tool::db::node_key_t>(101ull);
          file_node.name = "test.txt";
          file_node.data = flow_tool::db::File();
          Value<flow_tool::db::File>(file_node.data).blob = static_cast<flow_tool::db::blob_key_t>(201ull);
          fields.node.Add(file_node);
        }
        {
          flow_tool::db::Node root_node = Value(fields.node[flow_tool::db::node_key_t::RootNode]);
          Value<flow_tool::db::Dir>(root_node.data).dir.push_back(static_cast<flow_tool::db::node_key_t>(101ull));
          fields.node.Add(root_node);
        }
      })
      .Wait();

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::DirResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto dir_response = Value<flow_tool::api::success::DirResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://", dir_response.url);
    EXPECT_EQ("/", dir_response.path);
    ASSERT_EQ(1u, dir_response.dir.size());
    EXPECT_EQ("test.txt", dir_response.dir[0]);
  }

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/test.txt", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::FileResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto file_response = Value<flow_tool::api::success::FileResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://test.txt", file_response.url);
    EXPECT_EQ("/test.txt", file_response.path);
    EXPECT_EQ("Hello, World!\n", file_response.data);
  }
}

TEST(FlowTool, ReturnsAFileCreatedByPut) {
  current::time::ResetToZero();

  using flow_tool_t = flow_tool::FlowTool<flow_tool::db::FlowToolDB, StreamInMemoryStreamPersister>;

  flow_tool_t flow_tool(FLAGS_flow_tool_test_port, "");

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::DirResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto dir_response = Value<flow_tool::api::success::DirResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://", dir_response.url);
    EXPECT_EQ("/", dir_response.path);
    EXPECT_EQ(0u, dir_response.dir.size());
  }

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/yo.txt", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(404, static_cast<int>(response.code)) << response.body;
    EXPECT_EQ("ErrorPathNotFound", ParseJSON<flow_tool::api::error::ErrorBase>(response.body).error);
    const auto error = ParseJSON<flow_tool::api::error::ErrorPathNotFound>(response.body);
    EXPECT_EQ("ErrorPathNotFound", error.error);
    EXPECT_EQ("/yo.txt", error.path);
    EXPECT_EQ("yo.txt", error.not_found_component);
    EXPECT_EQ(0u, error.not_found_component_zero_based_index);
  }

  {
    flow_tool::api::request::PutRequest request_body;
    request_body.template Construct<flow_tool::api::request::PutFileRequest>("Yo, World!\n");

    const auto response = HTTP(PUT(Printf("http://localhost:%d/tree/yo.txt", FLAGS_flow_tool_test_port), request_body));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    // TODO(dkorolev): Response analysis.
    // Add the timestamp created/updated, number of versions already available, and time interval since last update?
  }

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::DirResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto dir_response = Value<flow_tool::api::success::DirResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://", dir_response.url);
    EXPECT_EQ("/", dir_response.path);
    ASSERT_EQ(1u, dir_response.dir.size());
    EXPECT_EQ("yo.txt", dir_response.dir[0]);
  }

  {
    const auto response = HTTP(GET(Printf("http://localhost:%d/tree/yo.txt", FLAGS_flow_tool_test_port)));
    EXPECT_EQ(200, static_cast<int>(response.code)) << response.body;
    const auto parsed_body = TryParseJSON<flow_tool::api::SuccessfulResponse, JSONFormat::JavaScript>(response.body);
    ASSERT_TRUE(Exists(parsed_body)) << response.body;
    ASSERT_TRUE(Exists<flow_tool::api::success::FileResponse>(Value(parsed_body))) << JSON(parsed_body);
    const auto file_response = Value<flow_tool::api::success::FileResponse>(Value(parsed_body));
    EXPECT_EQ("smoke_test_passed://yo.txt", file_response.url);
    EXPECT_EQ("/yo.txt", file_response.path);
    EXPECT_EQ("Yo, World!\n", file_response.data);
  }
}
