/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwcli_schema_test.cpp
 * @author Balaji Rajappa (balaji.rajappa@riftio.com) 
 * @date 2014/03/16
 * @brief Tests for RwCLI
 */

// Boost filesystem has some ABI issues with C++11 particularly related to
// scoped enums. Use this hack to make it work.
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#include "rwlib.h"
#include "rwcli_schema.hpp"
#include "rwut.h"
#include <poll.h>

namespace fs = boost::filesystem;
using namespace rw_yang;

const std::string TEST_SCHEMA_PATH {"tmp/cli_test/var/schema"};

class SchemaTestFixture: public ::testing::Test
{
public:
  static void SetUpTestCase() {
    const int overwrite = 1;
    // Add the test schema directory to the yang mod path
    fs::path mod_path1 = fs::path(getenv("RIFT_INSTALL")) /
                         fs::path(TEST_SCHEMA_PATH) /
                         fs::path("yang");
                         
    fs::path mod_path2 = fs::path(getenv("RIFT_INSTALL")) / 
                         fs::path(TEST_SCHEMA_PATH) /
                         fs::path("version") /
                         "confd_yang";
    std::string mod_path_env(getenv("YUMA_MODPATH"));
    std::string mod_path_str = mod_path1.native() + ":" +
                               mod_path2.native() + ":";
    mod_path_env.insert(0, mod_path_str.c_str());
    setenv("YUMA_MODPATH", mod_path_env.c_str(), overwrite);
  }

  static void TearDownTestCase() {
  }

  SchemaTestFixture() {
    schema_path_ = fs::path(getenv("RIFT_INSTALL")) / fs::path(TEST_SCHEMA_PATH);
    yang_path_   = schema_path_ / "yang";
    ver_path_    = schema_path_ / "version";
    fs::create_directories(yang_path_);
    fs::create_directories(ver_path_);

    test_yang_src_path_ = fs::path(getenv("RIFT_ROOT")) / 
                          "modules/core/mgmt/rwcli/test/yang_src";
  }

  ~SchemaTestFixture() {
    fs::remove_all(schema_path_);
  }

  virtual void SetUp() {
    // Constructors are preferred
  }

  virtual void TearDown(){
  }

  void copy_yang_file(const std::string& filename) {
    // The src files doesn't have yang extension. This is to prevent the
    // rift-shell from adding the src path to the YUMA_MOD_PATH
    fs::path src_file = test_yang_src_path_ / filename;
    fs::path dest_file = yang_path_ / filename;
    dest_file.replace_extension("yang");
    fs::copy_file(src_file, dest_file, fs::copy_option::fail_if_exists);
  }

  void copy_yang_to_ver(const std::string& filename, const std::string& ver_dir) {
    fs::path src_file = test_yang_src_path_ / filename;
    fs::path dest_file = ver_path_ / ver_dir / filename;
    dest_file.replace_extension("yang");
    fs::copy_file(src_file, dest_file, fs::copy_option::fail_if_exists);
  }

  void create_version_link(const std::string& ver_dir) {
    fs::path src = ver_path_ / ver_dir;
    fs::path link = ver_path_ / "confd_yang";
    fs::create_symlink(src, link);
  }

  fs::path schema_path_;
  fs::path yang_path_;
  fs::path ver_path_;

  fs::path test_yang_src_path_;
};

TEST_F(SchemaTestFixture, CreateDestroy) {
  TEST_DESCRIPTION("Test creation and destruction of SchemaManager");

  SchemaManager mgr(TEST_SCHEMA_PATH);

  ASSERT_NE(mgr.updater_, nullptr);

  int fd = mgr.updater_->get_handle();
  EXPECT_NE(fd, -1);
}

TEST_F(SchemaTestFixture, StartupSchema) {

  TEST_DESCRIPTION("Test the loading of all schemas during startup");

  copy_yang_file("schema1");
  copy_yang_file("schema2");
  copy_yang_file("schema3");

  SchemaManager mgr(TEST_SCHEMA_PATH);
  EXPECT_EQ(3, mgr.schema_table_.size());

  std::vector<std::string> test_schemas = { "schema1", "schema2", "schema3" };
  for (auto schema: test_schemas) {
    auto it = mgr.schema_table_.find(schema);
    EXPECT_NE(it, mgr.schema_table_.end());
    EXPECT_EQ(it->second.name_, schema);
  }
}

TEST_F(SchemaTestFixture, LoadSchema) {
  TEST_DESCRIPTION("Test Loading a sample schema using SchemaManager");

  const char* test_schema = "rwcli_test";
  SchemaManager mgr(TEST_SCHEMA_PATH);
  rw_status_t rc = mgr.load_schema(test_schema);

  ASSERT_EQ(RW_STATUS_SUCCESS, rc);
  EXPECT_EQ(1, mgr.schema_table_.size());

  auto it = mgr.schema_table_.find(test_schema);
  EXPECT_NE(it, mgr.schema_table_.end());
}

TEST_F(SchemaTestFixture, ChangeSetAdd) {
  TEST_DESCRIPTION("Test copying a schema file and see Changeset is updated");

  SchemaManager mgr(TEST_SCHEMA_PATH);

  copy_yang_file("schema1");

  // Check if an event is present, poll and return immediately
  struct pollfd fds[1] = {
    {mgr.updater_->get_handle(), POLLIN, 0}
  };
  int poll_ret = poll(fds, 1, 0);
  
  ASSERT_EQ(poll_ret, 1);

  mgr.updater_->check_for_updates();

  InotifySchemaUpdater *updater = static_cast<InotifySchemaUpdater*>(
                                      mgr.updater_.get());
  ASSERT_EQ(1, updater->changeset_.schema_files_.size());

  EXPECT_STREQ(updater->changeset_.schema_files_.front().filename_.c_str(),
               "schema1.yang");
}

TEST_F(SchemaTestFixture, ChangeSetAddMultiple) {
  TEST_DESCRIPTION("Test copying multiple schema files and see Changeset is updated");

  SchemaManager mgr(TEST_SCHEMA_PATH);

  copy_yang_file("schema1");
  copy_yang_file("schema2");
  copy_yang_file("schema3");

  // Check if an event is present, poll and return immediately
  struct pollfd fds[1] = {
    {mgr.updater_->get_handle(), POLLIN, 0}
  };
  int poll_ret = poll(fds, 1, 0);
  
  ASSERT_EQ(poll_ret, 1);

  mgr.updater_->check_for_updates();

  InotifySchemaUpdater *updater = static_cast<InotifySchemaUpdater*>(
                                      mgr.updater_.get());
  ASSERT_EQ(3, updater->changeset_.schema_files_.size());

  std::vector<std::string> expected= {"schema1.yang", "schema2.yang", "schema3.yang"};
  unsigned count = 0;
  for (auto sfile: updater->changeset_.schema_files_) {
    EXPECT_STREQ(sfile.filename_.c_str(), expected[count].c_str());
    count++;
  }
}

TEST_F(SchemaTestFixture, StampedSchema)
{
  TEST_DESCRIPTION("Copy a schema, create new version, stamp it and link it");

  SchemaManager mgr(TEST_SCHEMA_PATH);

  const char* ver = "00000001";
  const char* filename = "schema1";

  // Stamping is not required, create symbolic link is sufficent to check
  copy_yang_file(filename);
  fs::create_directory(ver_path_ / ver);
  copy_yang_to_ver(filename, ver);
  create_version_link(ver);

  // Check if an event is present, poll and return immediately
  struct pollfd fds[1] = {
    {mgr.updater_->get_handle(), POLLIN, 0}
  };

  int poll_ret = poll(fds, 1, 0);
  ASSERT_EQ(poll_ret, 1);
  mgr.updater_->check_for_updates();

  InotifySchemaUpdater *updater = static_cast<InotifySchemaUpdater*>(
                                      mgr.updater_.get());
  EXPECT_TRUE(updater->changeset_.schema_files_.empty()); 
  ASSERT_EQ(1, mgr.schema_table_.size());

  auto it = mgr.schema_table_.find(filename);
  EXPECT_NE(it, mgr.schema_table_.end());
  EXPECT_STREQ(it->second.name_.c_str(), "schema1");
}

TEST_F(SchemaTestFixture, StampedSchemaMultiple)
{
  TEST_DESCRIPTION("Copy multiple schema, create new version, stamp it and link it");

  SchemaManager mgr(TEST_SCHEMA_PATH);

  const char* ver = "00000001";
  std::vector<std::string> test_schemas = { "schema1", "schema2", "schema3" };

  fs::create_directory(ver_path_ / ver);
  for (auto schema: test_schemas) {
    // Stamping is not required, create symbolic link is sufficent to check
    copy_yang_file(schema);
    copy_yang_to_ver(schema, ver);
  }
  create_version_link(ver);

  // Check if an event is present, poll and return immediately
  struct pollfd fds[1] = {
    {mgr.updater_->get_handle(), POLLIN, 0}
  };

  int poll_ret = poll(fds, 1, 0);
  ASSERT_EQ(poll_ret, 1);
  mgr.updater_->check_for_updates();

  InotifySchemaUpdater *updater = static_cast<InotifySchemaUpdater*>(
                                      mgr.updater_.get());
  EXPECT_TRUE(updater->changeset_.schema_files_.empty()); 
  ASSERT_EQ(3, mgr.schema_table_.size());

  for (auto schema: test_schemas) {
    auto it = mgr.schema_table_.find(schema);
    EXPECT_NE(it, mgr.schema_table_.end());
    EXPECT_EQ(it->second.name_, schema);
  }

  // Create a new instance of SchemaManager and make sure it loads the schema1,
  // schema2, schema3 during startup.
  SchemaManager mgr_2(TEST_SCHEMA_PATH);
  ASSERT_EQ(3, mgr_2.schema_table_.size());

  for (auto schema: test_schemas) {
    auto it = mgr_2.schema_table_.find(schema);
    EXPECT_NE(it, mgr_2.schema_table_.end());
    EXPECT_EQ(it->second.name_, schema);
  }
}
