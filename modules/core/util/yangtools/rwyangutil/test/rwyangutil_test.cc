
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
* @file rwyangutil_test.cc
* @author Arun Muralidharan
* @date 01/10/2015
* @brief Google test cases for testing file protocol operations
* in dynamic schema app library
*
*/

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include "gmock/gmock.h"
#include "gtest/rw_gtest.h"
#include "rwut.h"
#include "rwyangutil.h"

//ATTN: Boost bug 10038
// https://svn.boost.org/trac/boost/ticket/10038
// Fixed in 1.57
// TODO: Remove the definition once upgraded to >= 1.57
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

namespace fs = boost::filesystem;

class FileProOpsTestsFixture : public ::testing::Test 
{
 public:
  FileProOpsTestsFixture()
  {
    rift_root_ = getenv("RIFT_INSTALL");
    if (!rift_root_.length()) {
      rift_root_ = "/";
    }

    schema_path_ = rift_root_ + "/" + DYNAMIC_SCHEMA_DIR;
    image_spath_ = rift_root_ + "/" + IMAGE_SCHEMA_DIR;
  }

  void SetUp()
  {
    if (fs::exists(schema_path_)) {
      TearDown(); // Just for first test case.
    }

    // Setup the schema directory.
    auto ret = std::system("rwyangutil --create-schema-dir");
    ASSERT_EQ (ret, 0);

    lock_file_ = std::string(rift_root_) + "/" + SCHEMA_LOCK_FILE;
    if (fs::exists(lock_file_)) {
      std::system("rwyangutil --lock-file-delete");
    }

    schema_tmp_loc_ = std::string(rift_root_) + "/" + SCHEMA_TMP_LOC;
    schema_ver_dir_ = std::string(rift_root_) + "/" + SCHEMA_VER_DIR;
  }

  void TearDown()
  {
    auto ret = std::system("rwyangutil --remove-schema-dir");
    ASSERT_EQ (ret, 0);
  }

  std::string rift_root_;
  std::string schema_path_;
  std::string lock_file_;
  std::string schema_tmp_loc_;
  std::string schema_ver_dir_;
  std::string image_spath_;
};

unsigned get_file_type_count(const std::string& path, const std::string& fext)
{
  unsigned count = 0;

  std::for_each(fs::directory_iterator(path),
                fs::directory_iterator(), [&count, &fext](const fs::directory_entry& et) 
                { if (et.path().extension().string() == fext) { count++; } });

  return count;
}

TEST_F(FileProOpsTestsFixture, SchemaDirCreationTest)
{
  // Check whether the schema directory is created properly
  ASSERT_TRUE ( fs::exists(schema_path_) ) ;
  ASSERT_TRUE ( fs::exists(schema_path_ + "/yang") );
  ASSERT_TRUE ( fs::exists(schema_path_ + "/fxs") );
  ASSERT_TRUE ( fs::exists(schema_path_ + "/xml") );
  ASSERT_TRUE ( fs::exists(schema_path_ + "/lib") );
  ASSERT_TRUE ( fs::exists(schema_path_ + "/lock") );
  ASSERT_TRUE ( fs::exists(schema_path_ + "/tmp") );
  ASSERT_TRUE ( fs::exists(schema_path_ + "/version") );

  // Check whether all the symbolic links are created properly.
  unsigned ycount = 0;
  fs::path ypath(schema_path_ + "/yang");
  for (fs::directory_iterator it(ypath);
       it != fs::directory_iterator();
       ++it) {

    ASSERT_TRUE( fs::is_symlink(it->path()) );
    EXPECT_STREQ ( it->path().extension().string().c_str(), ".yang");

    auto opath = fs::read_symlink( it->path() );
    EXPECT_FALSE (opath.empty());
    EXPECT_TRUE (fs::exists(opath));
    EXPECT_TRUE (fs::is_regular_file(opath));
    ycount++;
  }

  auto yc = get_file_type_count(image_spath_, std::string(".yang"));
  EXPECT_EQ (yc, ycount);
  std::cout << "Total installed yang files " << yc << std::endl;

  unsigned dcount = 0;
  fs::path xpath(schema_path_ + "/xml");
  for (fs::directory_iterator it(xpath);
       it != fs::directory_iterator();
       ++it) {

    ASSERT_TRUE( fs::is_symlink(it->path()) );
    EXPECT_STREQ ( it->path().extension().string().c_str(), ".dsdl");

    auto opath = fs::read_symlink( it->path() );
    EXPECT_FALSE (opath.empty());
    EXPECT_TRUE (fs::exists(opath));
    EXPECT_TRUE (fs::is_regular_file(opath));
    dcount++;
  }

  auto dc = get_file_type_count(image_spath_, std::string(".dsdl"));
  EXPECT_EQ (dc, dcount);
  std::cout << "Total installed dsdl files " << dc << std::endl;

  unsigned fcount = 0;
  fs::path fpath(schema_path_ + "/fxs");
  for (fs::directory_iterator it(fpath);
       it != fs::directory_iterator();
       ++it) {

    ASSERT_TRUE( fs::is_symlink(it->path()) );
    EXPECT_STREQ ( it->path().extension().string().c_str(), ".fxs");

    auto opath = fs::read_symlink( it->path() );
    EXPECT_FALSE (opath.empty());
    EXPECT_TRUE (fs::exists(opath));
    EXPECT_TRUE (fs::is_regular_file(opath));
    fcount++;
  }

  auto fc = get_file_type_count(image_spath_, std::string(".fxs"));
  EXPECT_EQ (fc, fcount);
  std::cout << "Total installed fxs files " << fc << std::endl;

  unsigned cmcount = 0;
  fs::path cpath(schema_path_ + "/cli");
  for (fs::directory_iterator it(cpath);
       it != fs::directory_iterator();
       ++it) {

    ASSERT_TRUE( fs::is_symlink(it->path()) );
    EXPECT_STREQ ( it->path().extension().string().c_str(), ".xml");

    auto opath = fs::read_symlink( it->path() );
    EXPECT_FALSE (opath.empty());
    EXPECT_TRUE (fs::exists(opath));
    EXPECT_TRUE (fs::is_regular_file(opath));
    cmcount++;
  }

  auto cm = get_file_type_count(image_spath_, std::string(".xml"));
  EXPECT_EQ (cm, cmcount);
  std::cout << "Total installed cli manifest files " << cm << std::endl;

  fs::path lpath(schema_path_ + "/lib");
  EXPECT_TRUE ( fs::is_empty(lpath) );

  fs::path lopath(schema_path_ + "/lock");
  EXPECT_TRUE ( fs::is_empty(lopath) );

  fs::path tpath(schema_path_ + "/tmp");
  EXPECT_TRUE ( fs::is_empty(tpath) );

  fs::path vpath(schema_path_ + "/version");
  int cnt = 0;
  std::for_each(fs::directory_iterator(vpath),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });
  ASSERT_EQ(cnt, 1);
}

TEST_F(FileProOpsTestsFixture, CreateAndDeleteLockFileTest)
{
  auto ret = std::system("rwyangutil --lock-file-create");
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(fs::exists(lock_file_));

  ret = std::system("rwyangutil --lock-file-delete");
  ASSERT_EQ(ret, 0);

  ASSERT_FALSE(fs::exists(lock_file_));
}

TEST_F(FileProOpsTestsFixture, CreateNewVersionDir)
{
  int cnt = 0;
  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });

  auto ret = std::system("rwyangutil --version-dir-create");
  ASSERT_EQ(ret, 0);

  int cnt2 = 0;
  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [&cnt2](const fs::directory_entry& _){ cnt2++; });

  ASSERT_EQ((cnt2 - cnt), 1);

  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [](const fs::directory_entry& e){ fs::remove_all(e); });

}

TEST_F(FileProOpsTestsFixture, CopyFilesFromTmp)
{
  //create tmp files for testing
  std::string cmd = std::string("touch ") + schema_tmp_loc_;
  auto fxs = cmd + "a.fxs";
  auto yang = fxs+ ";" + cmd + "a.yang";
  auto xml = yang + ";" + cmd + "a.dsdl";
  auto so = xml + ";" + cmd + "a.so";

  auto ret = std::system(so.c_str());
  ASSERT_EQ(ret, 0);

  ret = std::system("rwyangutil --copy-from-tmp");
  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/fxs/") + "a.fxs"));
  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/yang/") + "a.yang"));
  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/xml/") + "a.dsdl"));
  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/lib/") + "a.so"));

  ret = std::system("rwyangutil --tmp-file-cleanup");
  ASSERT_EQ(ret, 0);

  int cnt = 0;
  std::for_each(fs::directory_iterator(schema_tmp_loc_),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });
  ASSERT_EQ(cnt, 0);
}


TEST_F(FileProOpsTestsFixture, CascadeTest1)
{
  auto ret = std::system("rwyangutil --lock-file-create --version-dir-create");
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(fs::exists(lock_file_));

  int cnt = 0;
  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });
  ASSERT_TRUE(cnt > 0);

  ret = std::system("rwyangutil --lock-file-delete");
  ASSERT_EQ(ret, 0);
  ASSERT_FALSE(fs::exists(lock_file_));

  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [](const fs::directory_entry& e){ fs::remove_all(e); });
}

TEST_F(FileProOpsTestsFixture, CascadeTest2)
{
  //create tmp files for testing
  std::string cmd = std::string("touch ") + schema_tmp_loc_;
  auto fxs = cmd + "a.fxs";
  auto yang = fxs+ ";" + cmd + "a.yang";
  auto xml = yang + ";" + cmd + "a.dsdl";
  auto so = xml + ";" + cmd + "a.so";

  auto ret = std::system(so.c_str());
  ASSERT_EQ(ret, 0);

  cmd = std::string("rm -rf ") + schema_path_;
  fxs = cmd + "fxs/a.fxs";
  yang = fxs+ ";" + cmd + "yang/a.yang";
  xml = yang + ";" + cmd + "xml/a.dsdl";
  so = xml + ";" + cmd + "lib/a.so";

  ret = std::system(so.c_str());
  ASSERT_EQ(ret, 0);

  ret = std::system("rwyangutil --copy-from-tmp --tmp-file-cleanup");
  ASSERT_EQ(ret, 0);

  int cnt = 0;
  std::for_each(fs::directory_iterator(schema_tmp_loc_),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });
  ASSERT_EQ(cnt, 0);

  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/fxs/") + "a.fxs"));
  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/yang/") + "a.yang"));
  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/xml/") + "a.dsdl"));
  ASSERT_TRUE(fs::exists(schema_path_ + std::string("/lib/") + "a.so"));

  ret = std::system(so.c_str());
  ASSERT_EQ(ret, 0);
}

TEST_F(FileProOpsTestsFixture, InitDirTest)
{
  auto ret = std::system("rwyangutil --lock-file-create");
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(fs::exists(lock_file_));

  std::string cmd = std::string("touch ") + schema_tmp_loc_;
  auto fxs = cmd + "a.fxs";
  auto yang = fxs+ ";" + cmd + "a.yang";
  auto xml = yang + ";" + cmd + "a.dsdl";
  auto so = xml + ";" + cmd + "a.so";

  ret = std::system(so.c_str());
  ASSERT_EQ(ret, 0);

  ret = std::system("rwyangutil --init-schema-dir");
  ASSERT_EQ(ret, 0);
  ASSERT_FALSE(fs::exists(lock_file_));

  ASSERT_TRUE(fs::is_empty(schema_tmp_loc_));
}

TEST_F(FileProOpsTestsFixture, PruneDirTest)
{
  for (unsigned i = 0; i < 20; ++i) {
    auto ret = std::system("rwyangutil --version-dir-create");
    ASSERT_EQ(ret, 0);
  }

  auto ret = std::system("rwyangutil --prune-schema-dir");
  ASSERT_EQ(ret, 0);

  unsigned cnt = 0;
  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });

  ASSERT_EQ(cnt, 9);

  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [](const fs::directory_entry& et) { fs::remove(et.path().string()+"/stamp");});

  ret = std::system("rwyangutil --prune-schema-dir");
  ASSERT_EQ(ret, 0);

  cnt = 0;
  std::for_each(fs::directory_iterator(schema_ver_dir_),
                fs::directory_iterator(), [&cnt](const fs::directory_entry& _){ cnt++; });
  ASSERT_EQ(cnt, 2);
}

TEST_F(FileProOpsTestsFixture, RemoveConfdWSTest)
{
  auto ret1 = std::system("rwyangutil --rm-unique-confd-ws");
  auto ret2 = std::system("rwyangutil --rm-non-unique-confd-ws");
  ASSERT_EQ(ret1 && ret2, 0);

  std::for_each(fs::directory_iterator(rift_root_),
                fs::directory_iterator(), [](const fs::directory_entry& et) {
                                               auto name = et.path().filename().string();
                                               ASSERT_EQ (name.find(CONFD_WS_PREFIX), std::string::npos);
                                               ASSERT_EQ (name.find(CONFD_PWS_PREFIX), std::string::npos);
                                             });
}

TEST_F(FileProOpsTestsFixture, ArchiveConfdPersistWS)
{
  auto dir = rift_root_ + "/" + std::string("confd_persist_test");
  ASSERT_TRUE(fs::create_directory(dir));

  auto ret = std::system("rwyangutil --archive-confd-persist-ws");
  ASSERT_EQ(ret, 0);

  bool found = false;

  for (fs::directory_iterator entry(rift_root_);
        entry != fs::directory_iterator(); ++entry)
  {
    if (!fs::is_directory(entry->path())) continue;

    auto dir_name = entry->path().filename().string();
    auto pos = dir_name.find(AR_CONFD_PWS_PREFIX);

    if (pos != 0) continue;
    found = true;
    fs::remove(fs::path(rift_root_ + "/" + dir_name));
    break;
  }

  ASSERT_TRUE(found);
}
