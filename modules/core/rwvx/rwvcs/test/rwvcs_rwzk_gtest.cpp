
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



#include <rwut.h>

#include <rwvcs.h>
#include <rwvcs_internal.h>
#include <rwvcs_rwzk.h>
#include <rwvx.h>

using ::testing::HasSubstr;

static bool child_instance_name(rw_component_info * component, void *arg) {
  return !strncmp(component->instance_name, (char *)arg, strlen((char *)arg));
}

class RwvcsRwzkTest: public ::testing::Test {
  private:
    rwvx_instance_t * m_rwvx;
    rwvcs_instance_ptr_t m_rwvcs;

  protected:
    virtual void setup_tree() {
      rw_status_t status;
      rw_component_info cluster;
      rw_component_info parent;
      rw_component_info child1;
      rw_component_info child2;
      rw_component_info grandchild1;

      rw_component_info__init(&cluster);
      rw_component_info__init(&parent);
      rw_component_info__init(&child1);
      rw_component_info__init(&child2);
      rw_component_info__init(&grandchild1);


      cluster.instance_name = strdup("cluster-1");
      cluster.n_rwcomponent_children = 1;
      cluster.rwcomponent_children = (char**)malloc(sizeof(char *));
      cluster.rwcomponent_children[0] = strdup("parent-1");
      status = rwvcs_rwzk_node_create(m_rwvcs, &cluster);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      status = rwvcs_rwzk_node_update(m_rwvcs, &cluster);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);

      parent.instance_name = strdup("parent-1");
      parent.rwcomponent_parent = strdup("cluster-1");
      parent.has_component_type = true;
      parent.component_type = RWVCS_TYPES_COMPONENT_TYPE_RWVM;
      parent.vm_info = (rw_vm_info*)malloc(sizeof(rw_vm_info));
      rw_vm_info__init(parent.vm_info);
      parent.vm_info->has_leader = true;
      parent.vm_info->leader = true;

      parent.n_rwcomponent_children = 2;
      parent.rwcomponent_children = (char**)malloc(2 * sizeof(char*));
      parent.rwcomponent_children[0] = strdup("child-1");
      parent.rwcomponent_children[1] = strdup("child-2");
      status = rwvcs_rwzk_node_create(m_rwvcs, &parent);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      status = rwvcs_rwzk_node_update(m_rwvcs, &parent);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);

      child1.instance_name = strdup("child-1");
      child1.n_rwcomponent_children = 1;
      child1.rwcomponent_children = (char**)malloc(1 * sizeof(char*));
      child1.rwcomponent_children[0] = strdup("grandchild-1");
      child1.rwcomponent_parent = strdup("parent-1");
      status = rwvcs_rwzk_node_create(m_rwvcs, &child1);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      status = rwvcs_rwzk_node_update(m_rwvcs, &child1);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);

      child2.instance_name = strdup("child-2");
      child2.rwcomponent_parent = strdup("parent-1");
      status = rwvcs_rwzk_node_create(m_rwvcs, &child2);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      status = rwvcs_rwzk_node_update(m_rwvcs, &child2);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);

      grandchild1.instance_name = strdup("grandchild-1");
      grandchild1.rwcomponent_parent = strdup("child-1");
      status = rwvcs_rwzk_node_create(m_rwvcs, &grandchild1);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      status = rwvcs_rwzk_node_update(m_rwvcs, &grandchild1);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
    }

    virtual void SetUp() {
      rw_status_t status;

      m_rwvx = rwvx_instance_alloc();
      m_rwvcs = m_rwvx->rwvcs;

      status = rwcal_rwzk_zake_init(m_rwvx->rwcal_module);
      RW_ASSERT(status == RW_STATUS_SUCCESS);

      status = rwvcs_rwzk_seed_auto_instance(m_rwvcs, 100, NULL);
      RW_ASSERT(status == RW_STATUS_SUCCESS);

      status = rwcal_rwzk_create(m_rwvx->rwcal_module, "/rwvcs", NULL);
      RW_ASSERT(status == RW_STATUS_SUCCESS);
    }

    virtual void TearDown() {
      char ** children;

      rwcal_rwzk_get_children(m_rwvx->rwcal_module, "/rwvcs", &children, NULL);
      for (int i = 0; children[i]; ++i)
        rwvcs_rwzk_delete_node(m_rwvcs, children[i]);

      rwcal_rwzk_delete(m_rwvx->rwcal_module, "/rwvcs", NULL);

      return;
    }

    virtual void TestNonExistent() {
      bool found;

      found = rwvcs_rwzk_node_exists(m_rwvcs, "proc-1");
      ASSERT_FALSE(found);
    }

    virtual void TestDescendents() {
      rw_status_t status;
      char ** descendents;

      setup_tree();

      status = rwvcs_rwzk_descendents(m_rwvcs, "parent-1", &descendents);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);

      ASSERT_STREQ("parent-1", descendents[0]);
      ASSERT_STREQ("child-1", descendents[1]);
      ASSERT_STREQ("grandchild-1", descendents[2]);
      ASSERT_STREQ("child-2", descendents[3]);
      ASSERT_FALSE(descendents[4]);

      for (int i = 0; i < 4; ++i)
        free(descendents[i]);
      free(descendents);

      status = rwvcs_rwzk_descendents(m_rwvcs, "child-1", &descendents);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      ASSERT_STREQ("child-1", descendents[0]);
      ASSERT_STREQ("grandchild-1", descendents[1]);
      ASSERT_FALSE(descendents[2]);

      for (int i = 0; i < 2; ++i)
        free(descendents[i]);
      free(descendents);

      status = rwvcs_rwzk_descendents(m_rwvcs, "child-2", &descendents);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      ASSERT_STREQ("child-2", descendents[0]);
      ASSERT_FALSE(descendents[1]);

      for (int i = 0; i < 1; ++i)
        free(descendents[i]);
      free(descendents);

      status = rwvcs_rwzk_descendents(m_rwvcs, "child-1000", &descendents);
      ASSERT_EQ(status, RW_STATUS_NOTFOUND);
    }

    virtual void TestDescendentsComponent() {
      rw_status_t status;
      rw_component_info ** components;

      setup_tree();

      status = rwvcs_rwzk_descendents_component(m_rwvcs, "parent-1", &components);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);

      ASSERT_STREQ("parent-1", components[0]->instance_name);
      ASSERT_STREQ("child-1", components[1]->instance_name);
      ASSERT_STREQ("grandchild-1", components[2]->instance_name);
      ASSERT_STREQ("child-2", components[3]->instance_name);
      ASSERT_FALSE(components[4]);

      for (int i = 0; i < 4; ++i)
        protobuf_free(components[i]);
      free(components);

      status = rwvcs_rwzk_descendents_component(m_rwvcs, "child-1", &components);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      ASSERT_STREQ("child-1", components[0]->instance_name);
      ASSERT_STREQ("grandchild-1", components[1]->instance_name);
      ASSERT_FALSE(components[2]);

      for (int i = 0; i < 2; ++i)
        protobuf_free(components[i]);
      free(components);

      status = rwvcs_rwzk_descendents_component(m_rwvcs, "child-2", &components);
      ASSERT_EQ(status, RW_STATUS_SUCCESS);
      ASSERT_STREQ("child-2", components[0]->instance_name);
      ASSERT_FALSE(components[1]);

      for (int i = 0; i < 1; ++i)
        protobuf_free(components[i]);
      free(components);

      status = rwvcs_rwzk_descendents_component(m_rwvcs, "child-1000", &components);
      ASSERT_EQ(status, RW_STATUS_NOTFOUND);
    }


    virtual void TestNeighbors() {
      rw_status_t status;
      char ** neighbors;
      char search_str[] = "child";

      setup_tree();

      status = rwvcs_rwzk_get_neighbors(
          m_rwvcs,
          "parent-1",
          &child_instance_name,
          search_str,
          0,
          &neighbors);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("child-1", neighbors[0]);
      ASSERT_STREQ("child-2", neighbors[1]);
      ASSERT_FALSE(neighbors[2]);

      for (int i = 0; i < 2; ++i)
        free(neighbors[i]);
      free(neighbors);


      status = rwvcs_rwzk_get_neighbors(
          m_rwvcs,
          "grandchild-1",
          &child_instance_name,
          search_str,
          1,
          &neighbors);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("child-1", neighbors[0]);
      ASSERT_FALSE(neighbors[1]);

      free(neighbors[0]);
      free(neighbors);

      status = rwvcs_rwzk_get_neighbors(
          m_rwvcs,
          "grandchild-1",
          &child_instance_name,
          search_str,
          0,
          &neighbors);
      ASSERT_EQ(RW_STATUS_NOTFOUND, status);
      ASSERT_FALSE(neighbors);
    }

    virtual void TestClimbNeighbors() {
      rw_status_t status;
      char ** neighbors;
      char search[] = "grandchild";
      char missing[] = "youcannottouchthis";

      setup_tree();

      status = rwvcs_rwzk_climb_get_neighbors(
          m_rwvcs,
          "child-2",
          &child_instance_name,
          search,
          &neighbors);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("grandchild-1", neighbors[0]);
      ASSERT_FALSE(neighbors[1]);

      status = rwvcs_rwzk_climb_get_neighbors(
          m_rwvcs,
          "child-2",
          &child_instance_name,
          missing,
          &neighbors);
      ASSERT_EQ(RW_STATUS_NOTFOUND, status);
      ASSERT_FALSE(neighbors);

    }

    virtual void TestGetLeader() {
      rw_status_t status;
      char * leader;

      setup_tree();

      status = rwvcs_rwzk_get_leader(m_rwvcs, "cluster-1", &leader);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("parent-1", leader);
      free(leader);
    }

    virtual void TestNearestLeader() {
      rw_status_t status;
      char * leader;

      setup_tree();

      status = rwvcs_rwzk_nearest_leader(m_rwvcs, "child-1", &leader);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("parent-1", leader);
      free(leader);

      status = rwvcs_rwzk_nearest_leader(m_rwvcs, "grandchild-1", &leader);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("parent-1", leader);
      free(leader);

      status = rwvcs_rwzk_nearest_leader(m_rwvcs, "parent-1", &leader);
      ASSERT_EQ(RW_STATUS_SUCCESS, status);
      ASSERT_STREQ("parent-1", leader);
      free(leader);
    }

};

TEST_F(RwvcsRwzkTest, TestNonExistent) {
  TestNonExistent();
}

TEST_F(RwvcsRwzkTest, TestDescendents) {
  TestDescendents();
}

TEST_F(RwvcsRwzkTest, TestDescendentsComponent) {
  TestDescendentsComponent();
}

TEST_F(RwvcsRwzkTest, TestNeighbors) {
  TestNeighbors();
}

TEST_F(RwvcsRwzkTest, TestClimbNeighbors) {
  TestClimbNeighbors();
}

TEST_F(RwvcsRwzkTest, TestGetLeader) {
  TestGetLeader();
}

TEST_F(RwvcsRwzkTest, TestNearestLeader) {
  TestNearestLeader();
}



