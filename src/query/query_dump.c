/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/*
 * query_dump.c - Query processor printer
 */

#ident "$Id$"

#include "config.h"
#include <stdio.h>

#include "error_manager.h"
#include "object_representation.h"
#include "query_executor.h"
#include "class_object.h"
#include "system_parameter.h"
#include "scan_manager.h"
#include "perf_monitor.h"

#define foutput stdout

enum
{
  ARITH_EXP = 0,
  AGG_EXP = 1
};

#define HASH_NUMBER 128

#define HAVE_SUBQUERY_PROC(xasl_p) \
  ((xasl_p)->type != UNION_PROC \
   && (xasl_p)->type != INTERSECTION_PROC && (xasl_p)->type != DIFFERENCE_PROC)

#if defined(RYE_DEBUG)
typedef struct qdump_xasl_check_node QDUMP_XASL_CHECK_NODE;
struct qdump_xasl_check_node
{
  QDUMP_XASL_CHECK_NODE *next;
  UINTPTR xasl_addr;
  PROC_TYPE xasl_type;
  int referenced;
  int reachable;
};
#endif

static bool qdump_print_xasl_type (XASL_NODE * xasl);
static bool qdump_print_db_value_array (DB_VALUE ** array, int cnt);
static bool qdump_print_update_proc_node (UPDATE_PROC_NODE * ptr);
static bool qdump_print_delete_proc_node (DELETE_PROC_NODE * ptr);
static bool qdump_print_insert_proc_node (INSERT_PROC_NODE * ptr);
static const char *qdump_target_type_string (TARGET_TYPE type);
static const char *qdump_access_method_string (ACCESS_METHOD access);
static bool qdump_print_access_spec (ACCESS_SPEC_TYPE * spec_list);
static const char *qdump_key_range_string (RANGE range);
static bool qdump_print_key_info (KEY_INFO * key_info);
static const char *qdump_range_type_string (RANGE_TYPE range_type);
static bool qdump_print_index (INDX_INFO * indexptr);
static bool qdump_print_index_id (INDX_ID id);
static bool qdump_print_btid (BTID id);
static bool qdump_print_class (CLS_SPEC_TYPE * ptr);
static bool qdump_print_hfid (HFID id);
static bool qdump_print_vfid (VFID id);
static bool qdump_print_list (LIST_SPEC_TYPE * ptr);
static bool qdump_print_outlist (const char *title, OUTPTR_LIST * outlist);
static bool qdump_print_list_id (QFILE_LIST_ID * idptr);
static bool qdump_print_type_list (QFILE_TUPLE_VALUE_TYPE_LIST * typeptr);
static bool qdump_print_domain_list (int cnt, TP_DOMAIN ** ptr);
static bool qdump_print_sort_list (SORT_LIST * sorting_list);
static bool qdump_print_attribute_id (ATTR_DESCR attr);
static bool qdump_print_tuple_value_position (QFILE_TUPLE_VALUE_POSITION pos);
static bool qdump_print_value_list (VAL_LIST * vallist);
static bool qdump_print_regu_variable_list (REGU_VARIABLE_LIST varlist);
static const char *qdump_option_string (int option);
static bool qdump_print_db_value (DB_VALUE * value);
static const char *qdump_regu_type_string (REGU_DATATYPE type);
static bool qdump_print_regu_type (REGU_VARIABLE * value);
static bool qdump_print_value (REGU_VARIABLE * value);
static bool qdump_print_function_value (REGU_VARIABLE * regu);
static bool qdump_print_value_type_addr (REGU_VARIABLE * value);
static bool qdump_print_oid (OID oid);
static bool qdump_print_predicate (PRED_EXPR * predptr);
static const char *qdump_relation_operator_string (int op);
static const char *qdump_arith_operator_string (OPERATOR_TYPE opcode);
static bool qdump_print_arith_expression (ARITH_TYPE * arith_p);
static bool qdump_print_aggregate_expression (AGGREGATE_TYPE * aggptr);
static bool qdump_print_arith (int type, void *ptr);
static bool qdump_print_term (PRED_EXPR * pred_ptr);
static const char *qdump_bool_operator_string (BOOL_OP bool_op);
static bool qdump_print_lhs_predicate (PRED_EXPR * pred_p);
#if defined(RYE_DEBUG)
static QDUMP_XASL_CHECK_NODE *qdump_find_check_node_for (XASL_NODE * xasl,
                                                         QDUMP_XASL_CHECK_NODE * chk_nodes[HASH_NUMBER]);
static void qdump_check_node (XASL_NODE * xasl, QDUMP_XASL_CHECK_NODE * chk_nodes[HASH_NUMBER]);
static int qdump_print_inconsistencies (QDUMP_XASL_CHECK_NODE * chk_nodes[HASH_NUMBER]);
#endif /* RYE_DEBUG */

/*
 * qdump_print_xasl_type () -
 *   return:
 *   xasl(in):
 */
static bool
qdump_print_xasl_type (XASL_NODE * xasl_p)
{
  const char *type_string_p;

  switch (xasl_p->type)
    {
    case BUILDLIST_PROC:
      type_string_p = "buildlist_proc";
      break;
    case BUILDVALUE_PROC:
      type_string_p = "buildvalue_proc";
      break;
    case UNION_PROC:
      type_string_p = "union_proc";
      break;
    case DIFFERENCE_PROC:
      type_string_p = "difference_proc";
      break;
    case INTERSECTION_PROC:
      type_string_p = "intersection_proc";
      break;
    case SCAN_PROC:
      type_string_p = "scan_proc";
      break;
    case UPDATE_PROC:
      type_string_p = "update_proc";
      break;
    case DELETE_PROC:
      type_string_p = "delete_proc";
      break;
    case INSERT_PROC:
      type_string_p = "insert_proc";
      break;
    default:
      return false;
    }

  fprintf (foutput, "[%s:%p]\n", type_string_p, xasl_p);
  return true;
}

/*
 * qdump_print_db_value_array () -
 *   return:
 *   array(in)  :
 *   int cnt(in):
 */
static bool
qdump_print_db_value_array (DB_VALUE ** array_p, int count)
{
  int i;

  if (array_p == NULL)
    {
      return true;
    }

  for (i = 0; i < count; i++, array_p++)
    {
      if (!qdump_print_db_value (*array_p))
        {
          return false;
        }
      fprintf (foutput, "; ");
    }

  return true;
}

static bool
qdump_print_attribute (const char *action_p, int attr_count, int *attr_ids_p)
{
  int i;

  fprintf (foutput, "[number of attributes to %s:%d]", action_p, attr_count);
  fprintf (foutput, "[ID's of attributes for %s:", action_p);

  for (i = 0; i < attr_count; i++)
    {
      fprintf (foutput, "%d%c", attr_ids_p[i], i == attr_count - 1 ? ']' : ',');
    }

  return true;
}

/*
 * qdump_print_update_proc_node () -
 *   return:
 *   ptr(in):
 */
static bool
qdump_print_update_proc_node (UPDATE_PROC_NODE * node_p)
{
  UPDDEL_CLASS_INFO *cls = NULL;

  cls = node_p->class_info;

  qdump_print_oid (cls->class_oid);

  qdump_print_hfid (cls->class_hfid);

  qdump_print_attribute ("update", cls->no_attrs, cls->att_id);

  fprintf (foutput, "[numer of ORDER BY keys:%d]", node_p->no_orderby_keys);

  return true;
}

/*
 * qdump_print_delete_proc_node () -
 *   return:
 *   ptr(in):
 */
static bool
qdump_print_delete_proc_node (DELETE_PROC_NODE * node_p)
{
  UPDDEL_CLASS_INFO *cls = NULL;

  cls = node_p->class_info;

  qdump_print_oid (cls->class_oid);

  qdump_print_hfid (cls->class_hfid);

  return true;
}

/*
 * qdump_print_insert_proc_node () -
 *   return:
 *   ptr(in):
 */
static bool
qdump_print_insert_proc_node (INSERT_PROC_NODE * node_p)
{
  fprintf (foutput, "class oid[%d %d %d]", node_p->class_oid.pageid, node_p->class_oid.slotid, node_p->class_oid.volid);

  qdump_print_hfid (node_p->class_hfid);
  qdump_print_attribute ("insert", node_p->no_vals, node_p->att_id);

  return true;
}

/*
 * qdump_target_type_string () -
 *   return:
 *   type(in):
 */
static const char *
qdump_target_type_string (TARGET_TYPE type)
{
  switch (type)
    {
    case TARGET_CLASS:
      return "class";
    case TARGET_LIST:
      return "list";
    default:
      return "undefined";
    }
}

/*
 * qdump_access_method_string () -
 *   return:
 *   access(in):
 */
static const char *
qdump_access_method_string (ACCESS_METHOD access)
{
  switch (access)
    {
    case SEQUENTIAL:
      return "sequential";
    case INDEX:
      return "index";
    default:
      return "undefined";
    }
}

/*
 * qdump_print_access_spec () -
 *   return:
 *   spec_list(in):
 */
static bool
qdump_print_access_spec (ACCESS_SPEC_TYPE * spec_list_p)
{
  TARGET_TYPE type;

  if (spec_list_p == NULL)
    {
      return true;
    }

  type = spec_list_p->type;
  fprintf (foutput, " %s", qdump_target_type_string (type));

  fprintf (foutput, ",%s", qdump_access_method_string (spec_list_p->access));

  if (spec_list_p->access == INDEX)
    {
      if (qdump_print_index (spec_list_p->indexptr) == false)
        {
          return false;
        }
    }

  fprintf (foutput, "\n	");

  if (type == TARGET_CLASS)
    {
      qdump_print_class (&ACCESS_SPEC_CLS_SPEC (spec_list_p));
    }
  else if (type == TARGET_LIST)
    {
      qdump_print_list (&ACCESS_SPEC_LIST_SPEC (spec_list_p));
    }

  if (spec_list_p->where_key)
    {
      fprintf (foutput, "\n      key filter:");
      qdump_print_predicate (spec_list_p->where_key);
    }

  if (spec_list_p->where_pred)
    {
      fprintf (foutput, "\n      access pred:");
      qdump_print_predicate (spec_list_p->where_pred);
    }

  fprintf (foutput, ",fixed scan=%d", spec_list_p->fixed_scan);
  fprintf (foutput, ",qualified block=%d", spec_list_p->qualified_block);
  fprintf (foutput, ",fetch_type=%d", spec_list_p->fetch_type);

  fprintf (foutput, "\n-->next access spec:");
  qdump_print_access_spec (spec_list_p->next);
  fprintf (foutput, "\n");

  return true;
}

static const char *
qdump_key_range_string (RANGE range)
{
  switch (range)
    {
    case NA_NA:
      return "N/A";
    case GE_LE:
      return "GE_LE";
    case GE_LT:
      return "GE_LT";
    case GT_LE:
      return "GT_LE";
    case GT_LT:
      return "GT_LT";
    case GE_INF:
      return "GE_INF";
    case GT_INF:
      return "GT_INF";
    case INF_LT:
      return "INF_LT";
    case INF_LE:
      return "INF_LE";
    case INF_INF:
      return "INF_INF";
    case EQ_NA:
      return "EQ";
    default:
      return "undefined";
    }
}

/*
 * qdump_print_key_info () -
 *   return:
 *   key_info(in):
 */
static bool
qdump_print_key_info (KEY_INFO * key_info_p)
{
  int i;

  fprintf (foutput, "<key cnt:%d>", key_info_p->key_cnt);
  fprintf (foutput, "key ranges:");
  for (i = 0; i < key_info_p->key_cnt; i++)
    {
      fprintf (foutput, "<%s>", qdump_key_range_string (key_info_p->key_ranges[i].range));

      fprintf (foutput, "[");
      if (!qdump_print_value (key_info_p->key_ranges[i].key1))
        {
          return false;
        }

      fprintf (foutput, "][");

      if (!qdump_print_value (key_info_p->key_ranges[i].key2))
        {
          return false;
        }
      fprintf (foutput, "]");
    }
  fprintf (foutput, "<is constant:%d>", key_info_p->is_constant);

  fprintf (foutput, " key limit: [");
  qdump_print_value (key_info_p->key_limit_l);
  fprintf (foutput, "][");
  qdump_print_value (key_info_p->key_limit_u);
  fprintf (foutput, "][reset:%d]", key_info_p->key_limit_reset);

  return true;
}

static const char *
qdump_range_type_string (RANGE_TYPE range_type)
{
  switch (range_type)
    {
    case R_KEYLIST:
      return "R_KEYLIST";
    case R_RANGELIST:
      return "R_RANGELIST";
    default:
      assert (false);
      return "undefined";
    }
}

/*
 * qdump_print_index () -
 *   return:
 *   index_ptr(in):
 */
static bool
qdump_print_index (INDX_INFO * index_p)
{
  if (index_p == NULL)
    {
      return true;
    }

  fprintf (foutput, "<index id:");
  if (!qdump_print_index_id (index_p->indx_id))
    {
      return false;
    }
  fprintf (foutput, ">");

  fprintf (foutput, "<%s>", qdump_range_type_string (index_p->range_type));

  fprintf (foutput, "key info:");
  if (!qdump_print_key_info (&index_p->key_info))
    {
      return false;
    }
  fprintf (foutput, ">");

  return true;
}

/*
 * qdump_print_index_id () -
 *   return:
 *   id(in):
 */
static bool
qdump_print_index_id (INDX_ID id)
{
  if (id.type == T_BTID)
    {
      fprintf (foutput, "<type: Btree>");
      fprintf (foutput, "(%d;%d)", id.i.btid.vfid.fileid, id.i.btid.vfid.volid);
    }
  else
    {
      fprintf (foutput, "<type: Extendible Hashing>");
      fprintf (foutput, "<%d;%d;%d>", id.i.ehid.vfid.volid, id.i.ehid.vfid.fileid, id.i.ehid.pageid);
    }

  return true;
}

/*
 * qdump_print_btid () -
 *   return:
 *   id(in):
 */
static bool
qdump_print_btid (BTID id)
{
  fprintf (foutput, "<Btree:(%d;%d;%d)>", id.vfid.fileid, id.vfid.volid, id.root_pageid);
  return true;
}

/*
 * qdump_print_class () -
 *   return:
 *   ptr(in):
 */
static bool
qdump_print_class (CLS_SPEC_TYPE * class_p)
{
  qdump_print_hfid (class_p->hfid);
  fprintf (foutput, "oid[%d %d %d]", class_p->cls_oid.pageid, class_p->cls_oid.slotid, class_p->cls_oid.volid);
  fprintf (foutput, "\n	regu_list_key:");
  qdump_print_regu_variable_list (class_p->cls_regu_list_key);
  fprintf (foutput, "\n	regu_list_pred:");
  qdump_print_regu_variable_list (class_p->cls_regu_list_pred);
  fprintf (foutput, "\n	regu_list_rest:");
  qdump_print_regu_variable_list (class_p->cls_regu_list_rest);
  fprintf (foutput, "\n	regu_list_pk_next:");
  qdump_print_regu_variable_list (class_p->cls_regu_list_pk_next);
  return true;
}

/*
 * qdump_print_hfid () -
 *   return:
 *   id(in):
 */
static bool
qdump_print_hfid (HFID id)
{
  fprintf (foutput, "hfid:");
  qdump_print_vfid (id.vfid);
  fprintf (foutput, ":%d", id.hpgid);
  return true;
}

/*
 * qdump_print_vfid () -
 *   return:
 *   id(in):
 */
static bool
qdump_print_vfid (VFID id)
{
  fprintf (foutput, "vfid(%d;%d)", id.fileid, id.volid);
  return true;
}

/*
 * qdump_print_list () -
 *   return:
 *   ptr(in):
 */
static bool
qdump_print_list (LIST_SPEC_TYPE * list_p)
{
  fprintf (foutput, "list=");
  fprintf (foutput, "xasl:%p", list_p->xasl_node);
  fprintf (foutput, "\n	regu_list_pred:");
  qdump_print_regu_variable_list (list_p->list_regu_list_pred);
  fprintf (foutput, "\n	regu_list_rest:");
  qdump_print_regu_variable_list (list_p->list_regu_list_rest);
  return true;
}

/*
 * qdump_print_outlist () -
 *   return:
 *   title(in):
 *   outlist(in):
 */
static bool
qdump_print_outlist (const char *title_p, OUTPTR_LIST * outlist_p)
{
  REGU_VARIABLE_LIST nextptr;

  if (outlist_p == NULL)
    {
      return true;
    }

  nextptr = outlist_p->valptrp;
  fprintf (foutput, "-->%s:", title_p);

  while (nextptr)
    {
      fprintf (foutput, "[addr:%p]", &nextptr->value);
      if (!qdump_print_value (&nextptr->value))
        {
          return false;
        }

      fprintf (foutput, "; ");
      nextptr = nextptr->next;
    }

  fprintf (foutput, "\n");
  return true;
}

/*
 * qdump_print_list_id () -
 *   return:
 *   idptr(in):
 */
static bool
qdump_print_list_id (QFILE_LIST_ID * list_id_p)
{
  if (list_id_p == NULL)
    {
      return true;
    }

  fprintf (foutput, "(address:%p)", list_id_p);
  fprintf (foutput, "(type_list:");

  if (!qdump_print_type_list (&list_id_p->type_list))
    {
      return false;
    }

  fprintf (foutput, ")(tuple_cnt:%d)", list_id_p->tuple_cnt);
  return true;
}

/*
 * qdump_print_type_list () -
 *   return:
 *   typeptr(in):
 */
static bool
qdump_print_type_list (QFILE_TUPLE_VALUE_TYPE_LIST * type_list_p)
{
  fprintf (foutput, "<type_cnt:%d>", type_list_p->type_cnt);
  if (!qdump_print_domain_list (type_list_p->type_cnt, type_list_p->domp))
    {
      return false;
    }
  return true;
}

/*
 * qdump_print_domain_list () -
 *   return:
 *   cnt(in):
 *   ptr(in):
 */
static bool
qdump_print_domain_list (int cnt, TP_DOMAIN ** domains_p)
{
  int i;

  if (domains_p == NULL)
    {
      return true;
    }

  for (i = 0; i < cnt; i++)
    {
      fprintf (foutput, "%s; ", qdump_data_type_string (TP_DOMAIN_TYPE (domains_p[i])));
    }

  return true;
}

/*
 * qdump_print_sort_list () -
 *   return:
 *   sorting_list(in):
 */
static bool
qdump_print_sort_list (SORT_LIST * sort_list_p)
{
  if (sort_list_p == NULL)
    {
      return true;
    }

  fprintf (foutput, "<sorting field(POS):");
  if (!qdump_print_tuple_value_position (sort_list_p->pos_descr))
    {
      return false;
    }

  fprintf (foutput, ">");
  fprintf (foutput, "<sorting order:");
  if (sort_list_p->s_order == S_ASC)
    {
      fprintf (foutput, "ascending>");
    }
  else
    {
      fprintf (foutput, "descending>");
    }

  if (!qdump_print_sort_list (sort_list_p->next))
    {
      return false;
    }
  return true;
}

/*
 * qdump_print_attribute_id () -
 *   return:
 *   attr(in):
 */
static bool
qdump_print_attribute_id (ATTR_DESCR attr)
{
  fprintf (foutput, "attr_id:%d|db_type:", (int) attr.id);
  fprintf (foutput, "%s", qdump_data_type_string (attr.type));

  return true;
}

/*
 * qdump_print_tuple_value_position () -
 *   return:
 *   pos(in):
 */
static bool
qdump_print_tuple_value_position (QFILE_TUPLE_VALUE_POSITION pos)
{
  fprintf (foutput, "(position %d) (db_type ", pos.pos_no);
  fprintf (foutput, ")");

  return true;
}

/*
 * qdump_print_value_list () -
 *   return:
 *   vallist(in):
 */
static bool
qdump_print_value_list (VAL_LIST * value_list_p)
{
  QPROC_DB_VALUE_LIST dbval_list;

  if (value_list_p == NULL)
    {
      return true;
    }

  dbval_list = value_list_p->valp;
  fprintf (foutput, "(values %d <", value_list_p->val_cnt);

  while (dbval_list != NULL)
    {
      fprintf (foutput, "addr:%p|", dbval_list->val);
      fprintf (foutput, "type:%s", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (dbval_list->val)));
      fprintf (foutput, "|value:");

      if (!qdump_print_db_value (dbval_list->val))
        {
          return false;
        }

      fprintf (foutput, "; ");
      dbval_list = dbval_list->next;
    }

  fprintf (foutput, ">)");
  return true;
}

/*
 * qdump_print_regu_variable_list () -
 *   return:
 *   varlist(in):
 */
static bool
qdump_print_regu_variable_list (REGU_VARIABLE_LIST var_list)
{
  if (var_list == NULL)
    {
      return true;
    }

  while (var_list != NULL)
    {
      if (!qdump_print_value (&var_list->value))
        {
          return false;
        }

      fprintf (foutput, "; ");
      var_list = var_list->next;
    }

  return true;
}

/*
 * qdump_option_string () -
 *   return:
 *   option(in):
 */
static const char *
qdump_option_string (int option)
{
  switch (option)
    {
    case Q_DISTINCT:
      return "DISTINCT";
    case Q_ALL:
      return "ALL";
    default:
      return "undefined";
    }
}

/*
 * qdump_print_db_value () -
 *   return:
 *   value(in):
 */
static bool
qdump_print_db_value (DB_VALUE * value_p)
{
  db_value_print (value_p);
  return true;
}

static const char *
qdump_regu_type_string (REGU_DATATYPE type)
{
  switch (type)
    {
    case TYPE_DBVAL:
      return "TYPE_DBVAL";
    case TYPE_CONSTANT:
      return "TYPE_CONSTANT";
    case TYPE_ORDERBY_NUM:
      return "TYPE_ORDERBY_NUM";
    case TYPE_INARITH:
      return "TYPE_INARITH";
    case TYPE_OUTARITH:
      return "TYPE_OUTARITH";
    case TYPE_ATTR_ID:
      return "TYPE_ATTR_ID";
    case TYPE_POSITION:
      return "TYPE_POSITION";
    case TYPE_LIST_ID:
      return "TYPE_LIST_ID";
    case TYPE_POS_VALUE:
      return "TYPE_POS_VALUE";
    case TYPE_OID:
      return "TYPE_OID";
    case TYPE_FUNC:
      return "TYPE_FUNC";
    default:
      return "undefined";
    }
}

/*
 * qdump_print_regu_type () -
 *   return:
 *   value(in):
 */
static bool
qdump_print_regu_type (REGU_VARIABLE * value_p)
{
  DB_TYPE type;

  if (value_p->type == TYPE_DBVAL)
    {
      type = DB_VALUE_DOMAIN_TYPE (&(value_p->value.dbval));
      fprintf (foutput, "%s", qdump_data_type_string (type));
    }
  else
    {
      fprintf (foutput, "%s", qdump_regu_type_string (value_p->type));
    }

  return true;
}

/*
 * qdump_data_type_string () -
 *   return:
 *   type(in):
 */

const char *
qdump_data_type_string (DB_TYPE type)
{
  switch (type)
    {
    case DB_TYPE_NULL:
      return "NULL";
    case DB_TYPE_INTEGER:
      return "INTEGER";
    case DB_TYPE_BIGINT:
      return "BIGINT";
    case DB_TYPE_DOUBLE:
      return "DOUBLE";
    case DB_TYPE_VARCHAR:
      return "VARCHAR";
    case DB_TYPE_OBJECT:
      return "OBJECT";
    case DB_TYPE_SEQUENCE:
      return "SEQUENCE";
    case DB_TYPE_TIME:
      return "TIME";
    case DB_TYPE_DATETIME:
      return "DATETIME";
    case DB_TYPE_DATE:
      return "DATE";
    case DB_TYPE_VARIABLE:
      return "VARIABLE";
    case DB_TYPE_SUB:
      return "SUB";
    case DB_TYPE_OID:
      return "OID";
    case DB_TYPE_NUMERIC:
      return "NUMERIC";
    case DB_TYPE_VARBIT:
      return "VARBINARY";
    case DB_TYPE_RESULTSET:
      return "DB_RESULTSET";
    case DB_TYPE_TABLE:
      return "DB_TABLE";
    default:
      return "[***UNKNOWN***]";
    }
}

/*
 * qdump_print_value () -
 *   return:
 *   value(in):
 */
static bool
qdump_print_value (REGU_VARIABLE * value_p)
{
  XASL_NODE *xasl_p;

  if (value_p == NULL)
    {
      fprintf (foutput, "NIL");
      return true;
    }

  if (REGU_VARIABLE_IS_FLAGED (value_p, REGU_VARIABLE_HIDDEN_COLUMN))
    {
      fprintf (foutput, "[HIDDEN_COLUMN]");
    }
  xasl_p = REGU_VARIABLE_XASL (value_p);
  if (xasl_p)
    {
      fprintf (foutput, "[xasl:%p]", xasl_p);
    }

  fprintf (foutput, "[");
  qdump_print_value_type_addr (value_p);
  fprintf (foutput, "]");

  switch (value_p->type)
    {
    case TYPE_DBVAL:
      qdump_print_db_value (&value_p->value.dbval);
      return true;

    case TYPE_CONSTANT:
    case TYPE_ORDERBY_NUM:
      qdump_print_db_value (value_p->value.dbvalptr);
      return true;

    case TYPE_INARITH:
    case TYPE_OUTARITH:
      if (!qdump_print_arith (ARITH_EXP, (void *) value_p->value.arithptr))
        {
          return false;
        }
      return true;
    case TYPE_ATTR_ID:
      if (!qdump_print_attribute_id (value_p->value.attr_descr))
        {
          return false;
        }
      return true;

    case TYPE_LIST_ID:
      if (value_p->value.srlist_id->sorted)
        {
          fprintf (foutput, "[SORTED]");
        }
      else
        {
          fprintf (foutput, "[NOT SORTED]");
        }

      if (!qdump_print_list_id (value_p->value.srlist_id->list_id))
        {
          return false;
        }

      return true;

    case TYPE_POSITION:
      if (!qdump_print_tuple_value_position (value_p->value.pos_descr))
        {
          return false;
        }

      return true;

    case TYPE_POS_VALUE:
    case TYPE_OID:
      return true;

    case TYPE_FUNC:
      qdump_print_function_value (value_p);
      return true;

    default:
      return true;
    }
}

const char *
qdump_function_type_string (FUNC_TYPE ftype)
{
  switch (ftype)
    {
    case PT_MIN:
      return "MIN";
    case PT_MAX:
      return "MAX";
    case PT_SUM:
      return "SUM";
    case PT_AVG:
      return "AVG";
    case PT_STDDEV:
      return "STDDEV";
    case PT_STDDEV_POP:
      return "STDDEV_POP";
    case PT_STDDEV_SAMP:
      return "STDDEV_SAMP";
    case PT_VARIANCE:
      return "VARIANCE";
    case PT_VAR_POP:
      return "VAR_POP";
    case PT_VAR_SAMP:
      return "VAR_SAMP";
    case PT_COUNT:
      return "COUNT";
    case PT_COUNT_STAR:
      return "COUNT_STAR";
    case PT_GROUPBY_NUM:
      return "GROUPBY_NUM";
    case PT_TOP_AGG_FUNC:
      return "TOP_AGG_FUNC";
    case PT_GROUP_CONCAT:
      return "GROUP_CONCAT";
    case PT_GENERIC:
      return "GENERIC";
    case F_TOP_TABLE_FUNC:
      assert (false);           /* internal use only */
      return "F_TOP_TABLE_FUNC";
    case F_IDXKEY:
      return "F_IDXKEY";
    case F_SEQUENCE:
#if 0                           /* TODO - */
      assert (false);           /* should not reach here */
#endif
      return "F_SEQUENCE";
    case F_INSERT_SUBSTRING:
      return "INSERT_SUBSTRING";
    case F_ELT:
      return "ELT";
    default:
      return "***UNKNOWN***";
    }
}

/*
 * qdump_print_function_value () -
 *   return:
 *   regu(in):
 */
static bool
qdump_print_function_value (REGU_VARIABLE * regu_var_p)
{
  if (regu_var_p == NULL)
    {
      fprintf (foutput, "NIL");
      return true;
    }

  if (REGU_VARIABLE_IS_FLAGED (regu_var_p, REGU_VARIABLE_HIDDEN_COLUMN))
    {
      fprintf (foutput, "[HIDDEN_COLUMN]");
    }

  fprintf (foutput, "[TYPE_FUNC]");
  fprintf (foutput, "[%s]", qdump_function_type_string (regu_var_p->value.funcp->ftype));
  fprintf (foutput, "operand-->");
  qdump_print_regu_variable_list (regu_var_p->value.funcp->operand);

  return true;
}

/*
 * qdump_print_value_type_addr () -
 *   return:
 *   value(in):
 */
static bool
qdump_print_value_type_addr (REGU_VARIABLE * regu_var_p)
{
  void *addr;

  qdump_print_regu_type (regu_var_p);

  switch (regu_var_p->type)
    {
    case TYPE_DBVAL:
      addr = (void *) &regu_var_p->value.dbval;
      break;

    case TYPE_CONSTANT:
    case TYPE_ORDERBY_NUM:
      addr = (void *) regu_var_p->value.dbvalptr;
      break;

    case TYPE_INARITH:
    case TYPE_OUTARITH:
      addr = (void *) regu_var_p->value.arithptr;
      break;

    case TYPE_LIST_ID:
      addr = (void *) regu_var_p->value.srlist_id->list_id;
      break;

    case TYPE_ATTR_ID:
      addr = (void *) &regu_var_p->value.attr_descr;
      break;

    case TYPE_POSITION:
      addr = (void *) &regu_var_p->value.pos_descr;
      break;

    case TYPE_POS_VALUE:
      addr = (void *) &regu_var_p->value.val_pos;
      break;

    case TYPE_OID:
    case TYPE_FUNC:
      return true;

    default:
      return false;
    }

  fprintf (foutput, ":%p", addr);

  return true;
}


/*
 * qdump_print_oid () -
 *   return:
 *   id(in):
 */
static bool
qdump_print_oid (OID oid)
{
  fprintf (foutput, "[OID:%d,%d,%d]", oid.pageid, oid.slotid, oid.volid);
  return true;
}

static bool
qdump_print_comp_eval_term (EVAL_TERM * term_p)
{
  COMP_EVAL_TERM *et_comp_p = &term_p->et.et_comp;

  qdump_print_value (et_comp_p->comp_lhs);
  fprintf (foutput, " %s ", qdump_relation_operator_string (et_comp_p->comp_rel_op));

  if (et_comp_p->comp_rhs != NULL)
    {
      qdump_print_value (et_comp_p->comp_rhs);
    }

  return true;
}

static bool
qdump_print_alsm_eval_term (EVAL_TERM * term_p)
{
  ALSM_EVAL_TERM *et_alsm_p = &term_p->et.et_alsm;

  qdump_print_value (et_alsm_p->elem);
  fprintf (foutput, " %s ", qdump_relation_operator_string (et_alsm_p->alsm_rel_op));

  switch (et_alsm_p->eq_flag)
    {
    case F_SOME:
      fprintf (foutput, "some ");
      break;
    default:
      return false;
    }

  qdump_print_value (et_alsm_p->elemset);

  return true;
}

static bool
qdump_print_like_eval_term (EVAL_TERM * term_p)
{
  LIKE_EVAL_TERM *et_like_p = &term_p->et.et_like;

  fprintf (foutput, "SOURCE");
  qdump_print_value (et_like_p->src);
  fprintf (foutput, "PATTERN:");

  if (!qdump_print_value (et_like_p->pattern))
    {
      return false;
    }

  if (et_like_p->esc_char)
    {
      if (!qdump_print_value (et_like_p->esc_char))
        {
          return false;
        }
    }

  return true;
}

static bool
qdump_print_rlike_eval_term (EVAL_TERM * term_p)
{
  RLIKE_EVAL_TERM *et_rlike_p = &term_p->et.et_rlike;

  fprintf (foutput, "SOURCE");
  qdump_print_value (et_rlike_p->src);
  fprintf (foutput, (et_rlike_p->case_sensitive->value.dbval.data.i ?
                     "PATTERN (CASE SENSITIVE):" : "PATTERN (CASE INSENSITIVE):"));

  if (!qdump_print_value (et_rlike_p->pattern))
    {
      return false;
    }

  return true;
}

static bool
qdump_print_eval_term (PRED_EXPR * pred_p)
{
  EVAL_TERM *term = &pred_p->pe.eval_term;

  switch (term->et_type)
    {
    case T_COMP_EVAL_TERM:
      return qdump_print_comp_eval_term (term);

    case T_ALSM_EVAL_TERM:
      return qdump_print_alsm_eval_term (term);

    case T_LIKE_EVAL_TERM:
      return qdump_print_like_eval_term (term);

    case T_RLIKE_EVAL_TERM:
      return qdump_print_rlike_eval_term (term);

    default:
      return false;
    }
}

/*
 * qdump_print_term () -
 *   return:
 *   term(in):
 */
static bool
qdump_print_term (PRED_EXPR * pred_p)
{
  if (pred_p == NULL)
    {
      return true;
    }

  switch (pred_p->type)
    {
    case T_EVAL_TERM:
      return qdump_print_eval_term (pred_p);

    case T_NOT_TERM:
      fprintf (foutput, "(NOT ");

      if (!qdump_print_predicate (pred_p->pe.not_term))
        {
          return false;
        }
      fprintf (foutput, ")");

      return true;

    default:
      return false;
    }
}

static const char *
qdump_bool_operator_string (BOOL_OP bool_op)
{
  if (bool_op == B_AND)
    {
      return "AND";
    }
  else if (bool_op == B_OR)
    {
      return "OR";
    }
  else if (bool_op == B_XOR)
    {
      return "XOR";
    }
  else if (bool_op == B_IS)
    {
      return "IS";
    }
  else if (bool_op == B_IS_NOT)
    {
      return "IS NOT";
    }
  else
    {
      return "undefined";
    }
}

static bool
qdump_print_lhs_predicate (PRED_EXPR * pred_p)
{
  fprintf (foutput, "(");

  if (!qdump_print_predicate (pred_p->pe.pred.lhs))
    {
      return false;
    }

  fprintf (foutput, " %s ", qdump_bool_operator_string (pred_p->pe.pred.bool_op));

  return true;
}

/*
 * qdump_print_predicate () -
 *   return:
 *   predptr(in):
 */
static bool
qdump_print_predicate (PRED_EXPR * pred_p)
{
  int parn_cnt;

  if (pred_p == NULL)
    {
      return true;
    }

  switch (pred_p->type)
    {
    case T_PRED:
      if (qdump_print_lhs_predicate (pred_p) == false)
        {
          return false;
        }

      parn_cnt = 1;

      /* Traverse right-linear chains of AND/OR terms */
      for (pred_p = pred_p->pe.pred.rhs; pred_p->type == T_PRED; pred_p = pred_p->pe.pred.rhs)
        {
          if (qdump_print_lhs_predicate (pred_p) == false)
            {
              return false;
            }

          parn_cnt++;
        }

      /* rhs */
      switch (pred_p->type)
        {
        case T_EVAL_TERM:
        case T_NOT_TERM:
          if (!qdump_print_term (pred_p))
            {
              return false;
            }
          break;
        default:
          return false;
        }

      while (parn_cnt > 0)
        {
          fprintf (foutput, ")");
          parn_cnt--;
        }

      return true;

    case T_EVAL_TERM:
    case T_NOT_TERM:
      return qdump_print_term (pred_p);

    default:
      return false;
    }
}

/*
 * qdump_relation_operator_string () -
 *   return:
 *   op(in):
 */
static const char *
qdump_relation_operator_string (int op)
{
  switch (op)
    {
    case R_EQ:
      return "=";
    case R_NE:
      return "<>";
    case R_GT:
      return ">";
    case R_GE:
      return ">=";
    case R_LT:
      return "<";
    case R_LE:
      return "<=";
    case R_NULL:
      return "IS NULL";
    case R_EXISTS:
      return "EXISTS";
    case R_NULLSAFE_EQ:
      return "<=>";
    default:
      return "undefined";
    }
}

static const char *
qdump_arith_operator_string (OPERATOR_TYPE opcode)
{
  switch (opcode)
    {
    case T_ADD:
      return "+";
    case T_SUB:
      return "-";
    case T_MUL:
      return "*";
    case T_DIV:
      return "/";
    case T_MOD:
      return "mod";
    case T_STRCAT:
      return "||";
    case T_BIT_NOT:
      return "~";
    case T_BIT_AND:
      return "&";
    case T_BIT_OR:
      return "|";
    case T_BIT_XOR:
      return "^";
    case T_BITSHIFT_LEFT:
      return "<<";
    case T_BITSHIFT_RIGHT:
      return ">>";
    case T_INTDIV:
      return "div";
    default:
      return "undefined";
    }
}

static bool
qdump_print_arith_expression (ARITH_TYPE * arith_p)
{
  fprintf (foutput, "[%s]", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (arith_p->value)));

  if (arith_p->opcode == T_UNMINUS
#if defined(ENABLE_UNUSED_FUNCTION)
      || arith_p->opcode == T_UNPLUS
#endif /* ENABLE_UNUSED_FUNCTION */
    )
    {
      fprintf (foutput, "(");
      if (arith_p->opcode == T_UNMINUS)
        {
          fprintf (foutput, "-");
        }
#if defined(ENABLE_UNUSED_FUNCTION)
      else
        {
          fprintf (foutput, "+");
        }
#endif /* ENABLE_UNUSED_FUNCTION */

      if (!qdump_print_value (arith_p->rightptr))
        {
          return false;
        }
      fprintf (foutput, ")");
    }
  else
    {
      /*binary op */

      fprintf (foutput, "(");
      if (!qdump_print_value (arith_p->leftptr))
        {
          return false;
        }

      fprintf (foutput, "%s", qdump_arith_operator_string (arith_p->opcode));

      if (!qdump_print_value (arith_p->rightptr))
        {
          return false;
        }
      fprintf (foutput, ")");
    }

  return true;
}

static bool
qdump_print_aggregate_expression (AGGREGATE_TYPE * aggptr)
{
  fprintf (foutput, "[%s]", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (aggptr->accumulator.value)));

  fprintf (foutput, "%s(", qdump_function_type_string (aggptr->function));

  fprintf (foutput, "%s ", qdump_option_string (aggptr->option));

  if (!qdump_print_value (&aggptr->operand))
    {
      return false;
    }

  if (!qdump_print_list_id (aggptr->list_id))
    {
      return false;
    }

  fprintf (foutput, "(optimize:%d)", aggptr->flag_agg_optimize);

  if (!qdump_print_btid (aggptr->btid))
    {
      return false;
    }

  fprintf (foutput, ")");

  if (aggptr->next != NULL)
    {
      fprintf (foutput, "; ");
      if (!qdump_print_arith (AGG_EXP, aggptr->next))
        {
          return false;
        }
    }

  return true;
}

/*
 * qdump_print_arith () -
 *   return:
 *   type(in):
 *   ptr(in):
 */
static bool
qdump_print_arith (int type, void *ptr)
{
  if (ptr == NULL)
    {
      return true;
    }

  if (type == ARITH_EXP)
    {
      return qdump_print_arith_expression ((ARITH_TYPE *) ptr);
    }
  else if (type == AGG_EXP)
    {
      return qdump_print_aggregate_expression ((AGGREGATE_TYPE *) ptr);
    }

  return true;
}

#if defined(RYE_DEBUG)
/*
 * qdump_check_xasl_tree () -
 *   return:
 *   xasl(in):
 */
bool
qdump_check_xasl_tree (XASL_NODE * xasl_p)
{
  QDUMP_XASL_CHECK_NODE *chk_nodes[HASH_NUMBER] = { NULL };

  if (xasl_p == NULL)
    {
      return true;
    }

  /* recursively check the tree */
  qdump_check_node (xasl_p, chk_nodes);

  /* print any inconsistencies in the tree */
  return qdump_print_inconsistencies (chk_nodes);
}

/*
 * qdump_find_check_node_for () -
 *   return:
 *   xasl(in):
 *   chk_nodes(in):
 */
static QDUMP_XASL_CHECK_NODE *
qdump_find_check_node_for (XASL_NODE * xasl_p, QDUMP_XASL_CHECK_NODE * chk_nodes[HASH_NUMBER])
{
  UINTPTR access_node_hash;
  QDUMP_XASL_CHECK_NODE *check_node_p;

  access_node_hash = (UINTPTR) xasl_p % HASH_NUMBER;

  for (check_node_p = chk_nodes[access_node_hash]; check_node_p; check_node_p = check_node_p->next)
    {
      if (check_node_p->xasl_addr == (UINTPTR) xasl_p)
        {
          break;
        }
    }

  if (!check_node_p)
    {
      /* forward reference */
      check_node_p = (QDUMP_XASL_CHECK_NODE *) malloc (sizeof (QDUMP_XASL_CHECK_NODE));
      if (check_node_p == NULL)
        {
          return NULL;
        }
      check_node_p->next = chk_nodes[access_node_hash];
      chk_nodes[access_node_hash] = check_node_p;
      check_node_p->xasl_addr = (UINTPTR) xasl_p;
      check_node_p->xasl_type = xasl_p->type;
      check_node_p->referenced = 0;
      check_node_p->reachable = 0;
    }

  return check_node_p;
}

/*
 * qdump_check_node () -
 *   return:
 *   xasl(in):
 *   chk_nodes(in):
 */
static void
qdump_check_node (XASL_NODE * xasl_p, QDUMP_XASL_CHECK_NODE * chk_nodes[HASH_NUMBER])
{
  UINTPTR addr_hash;
  QDUMP_XASL_CHECK_NODE *check_node_p, *check_node1_p;
  ACCESS_SPEC_TYPE *spec_p;

  if (!xasl_p)
    {
      return;
    }

  /* get hash number */
  addr_hash = (UINTPTR) xasl_p % HASH_NUMBER;

  check_node_p = qdump_find_check_node_for (xasl_p, chk_nodes);
  if (check_node_p == NULL)
    {
      return;
    }

  if (check_node_p->reachable)
    {
      return;
    }

  check_node_p->reachable = 1;

  /*
   * Mark the node its access spec references.  You may need to create
   * it if it is a forward reference.
   */
  for (spec_p = xasl_p->spec_list; spec_p; spec_p = spec_p->next)
    {
      if (spec_p->type == TARGET_LIST)
        {
          check_node1_p = qdump_find_check_node_for (ACCESS_SPEC_XASL_NODE (spec_p), chk_nodes);
          /* mark as referenced */
          if (check_node1_p)
            {
              check_node1_p->referenced = 1;
            }
        }
    }

  /* recursively check the children of this node */
  switch (xasl_p->type)
    {
    case UNION_PROC:
    case DIFFERENCE_PROC:
    case INTERSECTION_PROC:
      check_node1_p = qdump_find_check_node_for (xasl_p->proc.union_.left, chk_nodes);
      if (check_node1_p)
        {
          check_node1_p->referenced = 1;
        }

      check_node1_p = qdump_find_check_node_for (xasl_p->proc.union_.right, chk_nodes);
      if (check_node1_p)
        {
          check_node1_p->referenced = 1;
        }
      break;

    case BUILDLIST_PROC:
      qdump_check_node (xasl_p->proc.buildlist.eptr_list, chk_nodes);

    default:
      break;
    }

  qdump_check_node (xasl_p->aptr_list, chk_nodes);
  qdump_check_node (xasl_p->bptr_list, chk_nodes);
  qdump_check_node (xasl_p->scan_ptr, chk_nodes);
  qdump_check_node (xasl_p->dptr_list, chk_nodes);
  qdump_check_node (xasl_p->fptr_list, chk_nodes);
  qdump_check_node (xasl_p->next, chk_nodes);
}

/*
 * qdump_print_inconsistencies () -
 *   return:
 *   chk_nodes(in):
 */
static int
qdump_print_inconsistencies (QDUMP_XASL_CHECK_NODE * chk_nodes[HASH_NUMBER])
{
  int i, error = 0;
  QDUMP_XASL_CHECK_NODE *check_node_p, *tmp_node_p;

  for (i = 0; i < HASH_NUMBER; i++)
    {
      for (check_node_p = chk_nodes[i]; check_node_p; check_node_p = check_node_p->next)
        {
          /* any buildlist procs that are referenced must be reachable */
          if (check_node_p->referenced && !check_node_p->reachable)
            {
              if (!error)
                {
                  fprintf (stdout, "\nSYSTEM ERROR--INCONSISTENT XASL TREE\n\n");
                }

              fprintf (stdout,
                       "Referenced node [%lld] is not reachable in the tree\n", (long long) check_node_p->xasl_addr);
              error = 1;
            }
        }
    }

  /* clean up our mallocs */
  for (i = 0; i < HASH_NUMBER; i++)
    {
      for (check_node_p = chk_nodes[i]; check_node_p; /* do nothing */ )
        {
          tmp_node_p = check_node_p;
          check_node_p = check_node_p->next;
          free_and_init (tmp_node_p);
        }
    }

  if (error)
    {
      fprintf (stdout, "\n");
    }

  return !error;
}
#endif /* RYE_DEBUG */

static bool
qdump_print_build_list_node (XASL_NODE * xasl_p)
{
  BUILDLIST_PROC_NODE *node_p = &xasl_p->proc.buildlist;

  if (xasl_p->outptr_list != NULL)
    {
      fprintf (foutput, "-->output columns:");
      qdump_print_db_value_array (node_p->output_columns, xasl_p->outptr_list->valptr_cnt);
      fprintf (foutput, "\n");
    }

  if (node_p->g_outptr_list)
    {
      qdump_print_outlist ("group by output ptrlist", node_p->g_outptr_list);
      fprintf (foutput, "\n");
    }

  if (node_p->groupby_list)
    {
      fprintf (foutput, "-->group by list:");
      qdump_print_sort_list (node_p->groupby_list);
      fprintf (foutput, "\n");
    }

  if (node_p->g_regu_list)
    {
      fprintf (foutput, "-->group by regu list:");
      qdump_print_regu_variable_list (node_p->g_regu_list);
      fprintf (foutput, "\n");
    }

  if (node_p->g_val_list)
    {
      fprintf (foutput, "-->group by val_list:");
      qdump_print_value_list (node_p->g_val_list);
      fprintf (foutput, "\n");
    }

  if (node_p->g_having_pred)
    {
      fprintf (foutput, "-->having predicate:");
      qdump_print_predicate (node_p->g_having_pred);
      fprintf (foutput, "\n");
    }

  if (node_p->g_grbynum_val)
    {
      fprintf (foutput, "-->grbynum val:");
      fprintf (foutput, "<addr:%p|", node_p->g_grbynum_val);
      fprintf (foutput, "type:%s", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (node_p->g_grbynum_val)));
      fprintf (foutput, "|value:");
      qdump_print_db_value (node_p->g_grbynum_val);
      fprintf (foutput, ">\n");
    }

  if (node_p->g_grbynum_pred)
    {
      fprintf (foutput, "-->grbynum predicate:");
      qdump_print_predicate (node_p->g_grbynum_pred);
      fprintf (foutput, "\n");

      if (node_p->g_grbynum_flag == XASL_G_GRBYNUM_FLAG_SCAN_CONTINUE)
        {
          fprintf (foutput, "-->grbynum CONTINUE\n");
        }
    }

  if (node_p->g_agg_list)
    {
      fprintf (foutput, "-->having agg list:");
      qdump_print_arith (AGG_EXP, (void *) node_p->g_agg_list);
      fprintf (foutput, "\n");
    }

  if (node_p->eptr_list)
    {
      fprintf (foutput, "-->EPTR LIST:%p\n", node_p->eptr_list);
    }

  if (node_p->g_with_rollup)
    {
      fprintf (foutput, "-->WITH ROLLUP\n");
    }

  return true;
}

static bool
qdump_print_build_value_node (XASL_NODE * xasl_p)
{
  BUILDVALUE_PROC_NODE *node_p = &xasl_p->proc.buildvalue;
  if (xasl_p->proc.buildvalue.having_pred)
    {
      fprintf (foutput, "-->having predicate:");
      qdump_print_predicate (node_p->having_pred);
      fprintf (foutput, "\n");
    }

  if (node_p->grbynum_val)
    {
      fprintf (foutput, "-->grbynum val:");
      fprintf (foutput, "<addr:%p|", node_p->grbynum_val);
      fprintf (foutput, "type:%s", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (node_p->grbynum_val)));
      fprintf (foutput, "|value:");
      qdump_print_db_value (node_p->grbynum_val);
      fprintf (foutput, ">\n");
    }

  if (node_p->agg_list)
    {
      fprintf (foutput, "-->agg list:");
      qdump_print_arith (AGG_EXP, (void *) node_p->agg_list);
      fprintf (foutput, "\n");
    }

  if (node_p->outarith_list)
    {
      fprintf (foutput, "-->outarith list:");
      qdump_print_arith (ARITH_EXP, (void *) node_p->outarith_list);
    }

  return true;
}

/*
 * qdump_print_xasl () -
 *   return:
 *   xasl(in):
 */
bool
qdump_print_xasl (XASL_NODE * xasl_p)
{
  VAL_LIST *single_tuple_p;
  QPROC_DB_VALUE_LIST value_list;
  int i;

  if (xasl_p == NULL)
    {
      return true;
    }

  fprintf (foutput, "\n<start of xasl structure %p>\n", xasl_p);
  qdump_print_xasl_type (xasl_p);

  if (xasl_p->flag)
    {
      int save_flag, nflag;

      save_flag = xasl_p->flag;
      nflag = 0;

      fprintf (foutput, "-->[flag=");

      if (XASL_IS_FLAGED (xasl_p, XASL_LINK_TO_REGU_VARIABLE))
        {
          XASL_CLEAR_FLAG (xasl_p, XASL_LINK_TO_REGU_VARIABLE);
          fprintf (foutput, "%sXASL_LINK_TO_REGU_VARIABLE", (nflag ? "|" : ""));
          nflag++;
        }

      if (XASL_IS_FLAGED (xasl_p, XASL_SKIP_ORDERBY_LIST))
        {
          XASL_CLEAR_FLAG (xasl_p, XASL_SKIP_ORDERBY_LIST);
          fprintf (foutput, "%sXASL_SKIP_ORDERBY_LIST", (nflag ? "|" : ""));
          nflag++;
        }

      if (XASL_IS_FLAGED (xasl_p, XASL_ZERO_CORR_LEVEL))
        {
          XASL_CLEAR_FLAG (xasl_p, XASL_ZERO_CORR_LEVEL);
          fprintf (foutput, "%sXASL_ZERO_CORR_LEVEL", (nflag ? "|" : ""));
          nflag++;
        }

      if (XASL_IS_FLAGED (xasl_p, XASL_TOP_MOST_XASL))
        {
          XASL_CLEAR_FLAG (xasl_p, XASL_TOP_MOST_XASL);
          fprintf (foutput, "%sXASL_TOP_MOST_XASL", (nflag ? "|" : ""));
          nflag++;
        }

      if (xasl_p->flag)
        {
          fprintf (foutput, "%d%s", xasl_p->flag, (nflag ? "|" : ""));
          nflag++;
        }

      fprintf (foutput, "]\n");

      xasl_p->flag = save_flag;
    }

  if (xasl_p->next)
    {
      fprintf (foutput, "-->next:%p\n", xasl_p->next);
    }

  if (xasl_p->list_id)
    {
      fprintf (foutput, "-->list id:");
      qdump_print_list_id (xasl_p->list_id);
      fprintf (foutput, "\n");
    }

  if (xasl_p->orderby_list)
    {
      fprintf (foutput, "-->order by list:");
      qdump_print_sort_list (xasl_p->orderby_list);
      fprintf (foutput, "\n");
    }

  if (xasl_p->ordbynum_val)
    {
      fprintf (foutput, "-->ordbynum val:");
      fprintf (foutput, "<addr:%p|", xasl_p->ordbynum_val);
      fprintf (foutput, "type:%s", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (xasl_p->ordbynum_val)));
      fprintf (foutput, "|value:");
      qdump_print_db_value (xasl_p->ordbynum_val);
      fprintf (foutput, ">\n");
    }

  if (xasl_p->ordbynum_pred)
    {
      fprintf (foutput, "-->ordbynum predicate:");
      qdump_print_predicate (xasl_p->ordbynum_pred);
      fprintf (foutput, "\n");
      if (xasl_p->ordbynum_flag == XASL_ORDBYNUM_FLAG_SCAN_CONTINUE)
        fprintf (foutput, "-->ordbynum CONTINUE\n");
    }

  if (xasl_p->orderby_limit)
    {
      const char *sort_limit = "disabled";
#if !defined(SERVER_MODE)
      const char *param = NULL;
      char value[64];
#endif

      fprintf (foutput, "-->orderby limit:");
      qdump_print_value (xasl_p->orderby_limit);

#if defined(SERVER_MODE)
      if (prm_get_bool_value (PRM_ID_USE_ORDERBY_SORT_LIMIT))
        {
          sort_limit = "enabled";
        }
#else
      param = prm_get_name (PRM_ID_USE_ORDERBY_SORT_LIMIT);
      if (param != NULL)
        {
          if (db_get_system_parameter_value (value, sizeof (value), param) == NO_ERROR)
            {
              if (value[0] == 'y')
                {
                  sort_limit = "enabled";
                }
            }
        }
#endif

      fprintf (foutput, " (optimization %s)", sort_limit);
      fprintf (foutput, "\n");
    }

  if (xasl_p->is_single_tuple)
    {
      fprintf (foutput, "-->single tuple:");
      single_tuple_p = xasl_p->single_tuple;
      if (single_tuple_p)
        {
          fprintf (foutput, "[value list]:");
          for (value_list = single_tuple_p->valp, i = 0;
               i < single_tuple_p->val_cnt; value_list = value_list->next, i++)
            {
              qdump_print_db_value (value_list->val);
              fprintf (foutput, "\t");
            }
        }
      fprintf (foutput, "\n");
    }

  if (xasl_p->option == Q_DISTINCT)
    {
      fprintf (foutput, "-->query DISTINCT\n");
    }

  if (xasl_p->outptr_list)
    {
      qdump_print_outlist ("outptr list", xasl_p->outptr_list);
      fprintf (foutput, "\n");
    }

  if (xasl_p->spec_list)
    {
      fprintf (foutput, "-->access spec:");
      qdump_print_access_spec (xasl_p->spec_list);
      fprintf (foutput, "\n");
    }

  if (xasl_p->val_list)
    {
      fprintf (foutput, "-->val_list:");
      qdump_print_value_list (xasl_p->val_list);
      fprintf (foutput, "\n");
    }

  if (xasl_p->aptr_list)
    {
      fprintf (foutput, "-->aptr list:%p\n", xasl_p->aptr_list);
    }

  if (xasl_p->scan_ptr)
    {
      fprintf (foutput, "-->scan ptr:%p\n", xasl_p->scan_ptr);
    }

  if (xasl_p->dptr_list)
    {
      fprintf (foutput, "-->dptr list:%p\n", xasl_p->dptr_list);
    }

  if (xasl_p->after_join_pred)
    {
      fprintf (foutput, "-->after_join predicate:");
      qdump_print_predicate (xasl_p->after_join_pred);
      fprintf (foutput, "\n");
    }

  if (xasl_p->if_pred)
    {
      fprintf (foutput, "-->if predicate:");
      qdump_print_predicate (xasl_p->if_pred);
      fprintf (foutput, "\n");
    }

  if (xasl_p->instnum_val)
    {
      fprintf (foutput, "-->instnum val:");
      fprintf (foutput, "<addr:%p|", xasl_p->instnum_val);
      fprintf (foutput, "type:%s", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (xasl_p->instnum_val)));
      fprintf (foutput, "|value:");
      qdump_print_db_value (xasl_p->instnum_val);
      fprintf (foutput, ">\n");
    }

  if (xasl_p->save_instnum_val)
    {
      fprintf (foutput, "-->old instnum val:");
      fprintf (foutput, "<addr:%p|", xasl_p->save_instnum_val);
      fprintf (foutput, "type:%s", qdump_data_type_string (DB_VALUE_DOMAIN_TYPE (xasl_p->save_instnum_val)));
      fprintf (foutput, "|value:");
      qdump_print_db_value (xasl_p->save_instnum_val);
      fprintf (foutput, ">\n");
    }

  if (xasl_p->instnum_pred)
    {
      fprintf (foutput, "-->instnum predicate:");
      qdump_print_predicate (xasl_p->instnum_pred);
      fprintf (foutput, "\n");
      if (xasl_p->instnum_flag == XASL_INSTNUM_FLAG_SCAN_CONTINUE)
        {
          fprintf (foutput, "-->instnum CONTINUE\n");
        }
    }

  fprintf (foutput, "-->current spec:");
  qdump_print_access_spec (xasl_p->curr_spec);
  fprintf (foutput, "\n");
  fprintf (foutput, "-->[next scan on=%d]", xasl_p->next_scan_on);
  fprintf (foutput, "[next scan block on=%d]", xasl_p->next_scan_block_on);
  fprintf (foutput, "\n");

  switch (xasl_p->type)
    {
    case BUILDLIST_PROC:
      qdump_print_build_list_node (xasl_p);
      break;

    case BUILDVALUE_PROC:
      qdump_print_build_value_node (xasl_p);
      break;

    case UNION_PROC:
    case DIFFERENCE_PROC:
    case INTERSECTION_PROC:
      fprintf (foutput, "left xasl:%p\n", xasl_p->proc.union_.left);
      fprintf (foutput, "right xasl:%p\n", xasl_p->proc.union_.right);
      break;

    case SCAN_PROC:
      break;

    case UPDATE_PROC:
      fprintf (foutput, "-->update info:");
      qdump_print_update_proc_node (&xasl_p->proc.update);
      fprintf (foutput, "\n");
      break;

    case DELETE_PROC:
      fprintf (foutput, "-->delete info:");
      qdump_print_delete_proc_node (&xasl_p->proc.delete_);
      fprintf (foutput, "\n");
      break;

    case INSERT_PROC:
      fprintf (foutput, "-->insert info:");
      qdump_print_insert_proc_node (&xasl_p->proc.insert);
      fprintf (foutput, "\n");
      break;

    default:
      return false;
    }

  fprintf (foutput, "end of internals of ");
  qdump_print_xasl_type (xasl_p);
  qdump_print_xasl (xasl_p->aptr_list);
  qdump_print_xasl (xasl_p->scan_ptr);
  qdump_print_xasl (xasl_p->dptr_list);

  if (xasl_p->type == BUILDLIST_PROC)
    {
      qdump_print_xasl (xasl_p->proc.buildlist.eptr_list);
    }

  qdump_print_xasl (xasl_p->next);
  fprintf (foutput, "creator OID:");
  qdump_print_oid (xasl_p->creator_oid);

  fprintf (foutput, "\nclass OID, #pages list:%d ", xasl_p->n_oid_list);
  for (i = 0; i < xasl_p->n_oid_list; ++i)
    {
      qdump_print_oid (xasl_p->class_oid_list[i]);
      fprintf (foutput, "/%d ", xasl_p->tcard_list[i]);
    }

  fprintf (foutput, "\ndbval_cnt:%d\n", xasl_p->dbval_cnt);
  fprintf (foutput, "<end of xasl structure %p>\n", xasl_p);
  fflush (foutput);

  return true;
}

/*
 * query trace dump for profile
 */

#if defined (SERVER_MODE)

/*
 * qdump_xasl_type_string () -
 *   return:
 *   xasl_p(in):
 */
static const char *
qdump_xasl_type_string (XASL_NODE * xasl_p)
{
  switch (xasl_p->type)
    {
    case BUILDLIST_PROC:
    case BUILDVALUE_PROC:
      return "SELECT";
    case SCAN_PROC:
      return "SCAN";
    case INSERT_PROC:
      return "INSERT";
    case UPDATE_PROC:
      return "UPDATE";
    case DELETE_PROC:
      return "DELETE";
    case UNION_PROC:
      return "UNION";
    case DIFFERENCE_PROC:
      return "DIFFERENCE";
    case INTERSECTION_PROC:
      return "INTERSECTION";
    default:
      assert (false);
      return "";
    }
}

/*
 * qdump_print_access_spec_stats () -
 *   return:
 *   spec_list_p(in):
 *   proc(in):
 */
static json_t *
qdump_print_access_spec_stats_json (ACCESS_SPEC_TYPE * spec_list_p)
{
  TARGET_TYPE type;
  char *class_name = NULL, *index_name = NULL;
  CLS_SPEC_TYPE *cls_node;
  ACCESS_SPEC_TYPE *spec;
  json_t *scan = NULL, *scan_array = NULL;
  int num_spec = 0;
  char spec_name[1024];

  for (spec = spec_list_p; spec != NULL; spec = spec->next)
    {
      num_spec++;
    }

  if (num_spec > 1)
    {
      scan_array = json_array ();
    }

  for (spec = spec_list_p; spec != NULL; spec = spec->next)
    {
      scan = json_object ();
      type = spec->type;

      if (type == TARGET_CLASS)
        {
          cls_node = &ACCESS_SPEC_CLS_SPEC (spec);
          class_name = heap_get_class_name (NULL, &(cls_node->cls_oid));
          spec_name[0] = '\0';

          if (spec->access == SEQUENTIAL)
            {
              if (class_name != NULL)
                {
                  sprintf (spec_name, "table (%s)", class_name);
                }
              else
                {
                  sprintf (spec_name, "table (unknown)");
                }
            }
          else if (spec->access == INDEX)
            {
              if (heap_get_indexname_of_btid (NULL, &(cls_node->cls_oid),
                                              &spec->indexptr->indx_id.i.btid, &index_name) == NO_ERROR)
                {
                  if (class_name != NULL && index_name != NULL)
                    {
                      sprintf (spec_name, "index (%s.%s)", class_name, index_name);
                    }
                  else
                    {
                      sprintf (spec_name, "index (unknown)");
                    }
                }
            }

          json_object_set_new (scan, "access", json_string (spec_name));

          if (class_name != NULL)
            {
              free_and_init (class_name);
            }
          if (index_name != NULL)
            {
              free_and_init (index_name);
            }
        }
      else if (type == TARGET_LIST)
        {
          json_object_set_new (scan, "access", json_string ("temp"));
        }

      scan_print_stats_json (&spec->s_id, scan);

      if (scan_array != NULL)
        {
          json_array_append_new (scan_array, scan);
        }
    }

  if (scan_array != NULL)
    {
      return scan_array;
    }
  else
    {
      return scan;
    }
}

/*
 * qdump_print_stats_json () -
 *   return:
 *   xasl_p(in):
 */
void
qdump_print_stats_json (XASL_NODE * xasl_p, json_t * parent)
{
  ORDERBY_STATS *ostats;
  GROUPBY_STATS *gstats;
  json_t *proc, *scan = NULL;
  json_t *subquery, *groupby, *orderby;
  json_t *left, *right;

  if (xasl_p == NULL || parent == NULL)
    {
      return;
    }

  if (xasl_p->type == SCAN_PROC)
    {
      proc = parent;
    }
  else
    {
      proc = json_object ();
      json_object_set_new (parent, qdump_xasl_type_string (xasl_p), proc);
    }

  switch (xasl_p->type)
    {
    case BUILDLIST_PROC:
    case BUILDVALUE_PROC:
    case UPDATE_PROC:
    case DELETE_PROC:
    case INSERT_PROC:
      json_object_set_new (proc, "time", json_integer (TO_MSEC (xasl_p->xasl_stats.elapsed_time)));
      json_object_set_new (proc, "fetch", json_integer (xasl_p->xasl_stats.fetches));
      json_object_set_new (proc, "ioread", json_integer (xasl_p->xasl_stats.ioreads));
      break;

    case UNION_PROC:
    case DIFFERENCE_PROC:
    case INTERSECTION_PROC:
      left = json_object ();
      right = json_object ();

      qdump_print_stats_json (xasl_p->proc.union_.left, left);
      qdump_print_stats_json (xasl_p->proc.union_.right, right);

      json_object_set_new (proc, "left", left);
      json_object_set_new (proc, "right", right);

      break;

    case SCAN_PROC:
    default:
      break;
    }

  if (xasl_p->spec_list != NULL)
    {
      scan = qdump_print_access_spec_stats_json (xasl_p->spec_list);
    }

  if (scan != NULL)
    {
      json_object_set_new (proc, "SCAN", scan);
      qdump_print_stats_json (xasl_p->scan_ptr, scan);
    }
  else
    {
      qdump_print_stats_json (xasl_p->scan_ptr, proc);
    }

  gstats = &xasl_p->groupby_stats;
  if (gstats->run_groupby)
    {
      groupby = json_object ();

      json_object_set_new (groupby, "time", json_integer (TO_MSEC (gstats->groupby_time)));

      if (gstats->groupby_sort)
        {
          json_object_set_new (groupby, "sort", json_true ());
          json_object_set_new (groupby, "page", json_integer (gstats->groupby_pages));
          json_object_set_new (groupby, "ioread", json_integer (gstats->groupby_ioreads));
        }
      else
        {
          json_object_set_new (groupby, "sort", json_false ());
        }

      json_object_set_new (groupby, "rows", json_integer (gstats->rows));
      json_object_set_new (proc, "GROUPBY", groupby);
    }

  ostats = &xasl_p->orderby_stats;
  if (ostats->orderby_filesort || ostats->orderby_topnsort || XASL_IS_FLAGED (xasl_p, XASL_SKIP_ORDERBY_LIST))
    {
      orderby = json_object ();

      json_object_set_new (orderby, "time", json_integer (TO_MSEC (ostats->orderby_time)));

      if (ostats->orderby_filesort)
        {
          json_object_set_new (orderby, "sort", json_true ());
          json_object_set_new (orderby, "page", json_integer (ostats->orderby_pages));
          json_object_set_new (orderby, "ioread", json_integer (ostats->orderby_ioreads));
        }
      else if (ostats->orderby_topnsort)
        {
          json_object_set_new (orderby, "topnsort", json_true ());
        }
      else
        {
          json_object_set_new (orderby, "skipsort", json_true ());
        }

      json_object_set_new (proc, "ORDERBY", orderby);
    }

  if (HAVE_SUBQUERY_PROC (xasl_p) && xasl_p->aptr_list != NULL)
    {
      if (HAVE_SUBQUERY_PROC (xasl_p->aptr_list))
        {
          subquery = json_object ();
          qdump_print_stats_json (xasl_p->aptr_list, subquery);
          json_object_set_new (proc, "SUBQUERY (uncorrelated)", subquery);
        }
      else
        {
          qdump_print_stats_json (xasl_p->aptr_list, proc);
        }
    }

  if (xasl_p->dptr_list != NULL)
    {
      subquery = json_object ();
      qdump_print_stats_json (xasl_p->dptr_list, subquery);
      json_object_set_new (proc, "SUBQUERY (correlated)", subquery);
    }
}

/*
 * qdump_print_access_spec_stats_text () -
 *   return:
 *   fp(in):
 *   spec_list_p(in):
 */
static void
qdump_print_access_spec_stats_text (FILE * fp, ACCESS_SPEC_TYPE * spec_list_p, int indent)
{
  TARGET_TYPE type;
  char *class_name = NULL, *index_name = NULL;
  CLS_SPEC_TYPE *cls_node;
  ACCESS_SPEC_TYPE *spec;
  int i, multi_spec_indent;

  if (spec_list_p == NULL)
    {
      return;
    }

  multi_spec_indent = fprintf (fp, "%*cSCAN ", indent, ' ');

  for (spec = spec_list_p, i = 0; spec != NULL; spec = spec->next, i++)
    {
      if (i > 0)
        {
          fprintf (fp, "%*c", multi_spec_indent, ' ');
        }

      type = spec->type;
      if (type == TARGET_CLASS)
        {
          cls_node = &ACCESS_SPEC_CLS_SPEC (spec);
          class_name = heap_get_class_name (NULL, &(cls_node->cls_oid));

          if (spec->access == SEQUENTIAL)
            {
              if (class_name != NULL)
                {
                  fprintf (fp, "(table: %s), ", class_name);
                }
              else
                {
                  fprintf (fp, "(table: unknown), ");
                }
            }
          else if (spec->access == INDEX)
            {
              if (heap_get_indexname_of_btid (NULL, &(cls_node->cls_oid),
                                              &spec->indexptr->indx_id.i.btid, &index_name) == NO_ERROR)
                {
                  if (class_name != NULL && index_name != NULL)
                    {
                      fprintf (fp, "(index: %s.%s), ", class_name, index_name);
                    }
                  else
                    {
                      fprintf (fp, "(index: unknown), ");
                    }
                }
            }

          scan_print_stats_text (fp, &spec->s_id);

          if (class_name != NULL)
            {
              free_and_init (class_name);
            }
          if (index_name != NULL)
            {
              free_and_init (index_name);
            }
        }
      else
        {
          scan_print_stats_text (fp, &spec->s_id);
        }

      fprintf (fp, "\n");
    }

  return;
}

/*
 * qdump_print_stats_text () -
 *   return:
 *   fp(in):
 *   xasl_p(in):
 *   indent(in):
 */
void
qdump_print_stats_text (FILE * fp, XASL_NODE * xasl_p, int indent)
{
  ORDERBY_STATS *ostats;
  GROUPBY_STATS *gstats;

  if (xasl_p == NULL)
    {
      return;
    }

  indent += 2;

  if (xasl_p->type != SCAN_PROC)
    {
      fprintf (fp, "%*c", indent, ' ');
    }

  switch (xasl_p->type)
    {
    case BUILDLIST_PROC:
    case BUILDVALUE_PROC:
    case INSERT_PROC:
    case UPDATE_PROC:
    case DELETE_PROC:
      fprintf (fp, "%s (time: %d, fetch: %lld, ioread: %lld)\n",
               qdump_xasl_type_string (xasl_p),
               TO_MSEC (xasl_p->xasl_stats.elapsed_time),
               (long long int) xasl_p->xasl_stats.fetches, (long long int) xasl_p->xasl_stats.ioreads);
      indent += 2;
      break;

    case UNION_PROC:
      fprintf (fp, "UNION\n");
      qdump_print_stats_text (fp, xasl_p->proc.union_.left, indent);
      qdump_print_stats_text (fp, xasl_p->proc.union_.right, indent);
      break;
    case DIFFERENCE_PROC:
      fprintf (fp, "DIFFERENCE\n");
      qdump_print_stats_text (fp, xasl_p->proc.union_.left, indent);
      qdump_print_stats_text (fp, xasl_p->proc.union_.right, indent);
      break;
    case INTERSECTION_PROC:
      fprintf (fp, "INTERSECTION\n");
      qdump_print_stats_text (fp, xasl_p->proc.union_.left, indent);
      qdump_print_stats_text (fp, xasl_p->proc.union_.right, indent);
      break;

    case SCAN_PROC:
    default:
      break;
    }

  qdump_print_access_spec_stats_text (fp, xasl_p->spec_list, indent);

  qdump_print_stats_text (fp, xasl_p->scan_ptr, indent);

  gstats = &xasl_p->groupby_stats;
  if (gstats->run_groupby)
    {
      fprintf (fp, "%*c", indent, ' ');
      fprintf (fp, "GROUPBY (time: %d", TO_MSEC (gstats->groupby_time));

      if (gstats->groupby_sort)
        {
          fprintf (fp, ", sort: true, page: %lld, ioread: %lld",
                   (long long int) gstats->groupby_pages, (long long int) gstats->groupby_ioreads);
        }
      else
        {
          fprintf (fp, ", sort: false");
        }

      fprintf (fp, ", rows: %d)\n", gstats->rows);
    }

  ostats = &xasl_p->orderby_stats;
  if (ostats->orderby_filesort || ostats->orderby_topnsort || XASL_IS_FLAGED (xasl_p, XASL_SKIP_ORDERBY_LIST))
    {
      fprintf (fp, "%*c", indent, ' ');
      fprintf (fp, "ORDERBY (time: %d", TO_MSEC (ostats->orderby_time));

      if (ostats->orderby_filesort)
        {
          fprintf (fp, ", sort: true");
          fprintf (fp, ", page: %lld, ioread: %lld",
                   (long long int) ostats->orderby_pages, (long long int) ostats->orderby_ioreads);
        }
      else if (ostats->orderby_topnsort)
        {
          fprintf (fp, ", topnsort: true");
        }
      else
        {
          fprintf (fp, ", skipsort: true");
        }
      fprintf (fp, ")\n");
    }

  if (HAVE_SUBQUERY_PROC (xasl_p) && xasl_p->aptr_list != NULL)
    {
      if (HAVE_SUBQUERY_PROC (xasl_p->aptr_list))
        {
          fprintf (fp, "%*cSUBQUERY (uncorrelated)\n", indent, ' ');
        }

      qdump_print_stats_text (fp, xasl_p->aptr_list, indent);
    }

  if (xasl_p->dptr_list != NULL)
    {
      fprintf (fp, "%*cSUBQUERY (correlated)\n", indent, ' ');
      qdump_print_stats_text (fp, xasl_p->dptr_list, indent);
    }
}
#endif /* SERVER_MODE */
