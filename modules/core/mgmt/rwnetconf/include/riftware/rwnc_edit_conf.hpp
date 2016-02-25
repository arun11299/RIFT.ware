
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_instance.hpp
 * @author Vinod Kamalaraj
 * @date 2014/06/25
 * @brief RW.Netconf Edit configuration parsed node object
 */

#ifndef RWNC_EDIT_CONF_HPP_
#define RWNC_EDIT_CONF_HPP_

#include <yangmodel.h>
#include <rw_xml.h>

struct rw_ncclnt_pvt_edit_cfg_s;
typedef struct rw_ncclnt_pvt_edit_cfg_s  rw_ncclnt_pvt_edit_cfg_t;
typedef struct rw_ncclnt_pvt_edit_cfg_s *rw_ncclnt_pvt_edit_cfg_ptr_t;

struct rw_ncclnt_pvt_edit_op_s;
typedef struct rw_ncclnt_pvt_edit_op_s  rw_ncclnt_pvt_edit_op_t;
typedef struct rw_ncclnt_pvt_edit_op_s *rw_ncclnt_pvt_edit_op_ptr_t;

namespace rw_netconf {


class EditConfOperation;
class EditConf;

class EditConfOpIter;

/**
 * @defgroup RwNcEditConfig RW.Netconf Edit Configuration Object
 * The <configuration> XML Node that is part of a NETCONF RPC represents  
 * specific operations to be performed on specific nodes in the target
 * configuration object. The EditConf object represents all operations that
 * are specified in the configuration node. Each individual operation is
 * specified as a EditConfOperation.
 *
 * The protocol does not limit the operations that are available on the nodes.
 * The protocol specifies that multiple operations on the same nodes will result
 * in unpredictable results.
 *
 * The edit config request will not be completely validated by this parsing.
 * The first operation on a subtree will be performed, and any subsequent
 * operations present in the subtree will be ignored. As an example,
 *
 * <config>
 *    <node xc:operation=merge>
 *       <name> a </name>
 *          <port xc:operation=delete>
 *            <id> 0 </id>
 *          <port>
 *             <id> 1 </id>
 *             <foo> some_foo_val </foo>
 *          </port>
 *   </name>
 *  </config>
 *
 *  In the above example, the parsing function will NOT discover the delete
 *  operation. 
 *       
 * When multiple operations are specified on the same nodes, but in different
 * XML paths under configuration, the operations will be parsed by the parsing
 * function. The implementing function will decide on the order in which the
 * operations are performed. The parsing function does not guarentee any order
 * in which sibling nodes appear in the parsed output, as this is not required
 * by the netconf protocol
 *
 * Consider the following example:
 *
 *  <config>
 *    <node>
 *       <name> a </name>
 *          <port xc:operation=create>
 *            <id> 0 </id>
 *          <port xc:operation=delete>
 *             <id> 0 </id>
 *          </port>
 *   </name>
 *
 * The parsing function will detect 2 operations, a create and a delete, but the
 * order of operations in the returned parsed object is not predictable.
 */

class EditConfigOperation
{
 public:
  /**
   * The path to the node on which the operation is to be performed on. The path
   * 
   */
  rw_yang_path_element_t *path;
};

/** @} */
} // namespace rw_netconf
#endif // RWNC_EDIT_CONF_HPP_



