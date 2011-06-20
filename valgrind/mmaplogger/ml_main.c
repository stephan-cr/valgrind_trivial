/*
   Mmap Logger

   Copyright (C) 2006, 2011 Stephan Creutz

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "pub_tool_basics.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_tooliface.h"
#include "vki/vki-linux.h"

static VgHashTable mmaps;

/* first two fields have to match VgHashNode */
struct AddrNode {
  struct AddrNode *next;
  UWord key;
  SizeT len;
};

static void ml_post_clo_init(void)
{
  mmaps = VG_(HT_construct)("ml-map");
  tl_assert(NULL != mmaps);
}

static void check_mmap(Addr addr, SizeT size)
{
  Addr current_addr = addr;
  SizeT current_size = size;
  while (current_size > 0) {
    Addr roundeddn_addr = VG_PGROUNDDN(current_addr);
    struct AddrNode *node =
      VG_(HT_lookup)(mmaps, (UWord) roundeddn_addr);
    if ((node != NULL) && (current_addr < node->key + node->len))
      VG_(message)(Vg_UserMsg,
                   "write to mmaped memory to page 0x%lx\n",
                   roundeddn_addr);
    Addr roundedup_addr = VG_PGROUNDUP(current_addr + 1);
    SizeT chunk = roundedup_addr - current_addr;
    current_size = chunk > current_size ? 0 : current_size - chunk;
    current_addr = roundedup_addr;
  }
}

static IRSB* ml_instrument(VgCallbackClosure* closure, IRSB* bb,
                           VexGuestLayout* layout, VexGuestExtents* vge,
                           IRType gWordTy, IRType hWordTy)
{
  if (gWordTy != hWordTy)
    VG_(tool_panic)("host/guest word size mismatch");

  IRSB *new_bb = deepCopyIRSBExceptStmts(bb);

  Int i = 0;
  for (; i < bb->stmts_used && bb->stmts[i]->tag != Ist_IMark; i++)
    addStmtToIRSB(new_bb, bb->stmts[i]);

  for (; i < bb->stmts_used; i++) {
    IRStmt *st = bb->stmts[i];
    tl_assert(st);
    addStmtToIRSB(new_bb, st);
    if (st->tag == Ist_Store) {
      IRExpr *addr_expr = st->Ist.Store.addr;
      IRExpr *data_expr = st->Ist.Store.data;
      tl_assert2(isIRAtom(addr_expr) && isIRAtom(data_expr),
          "store args are atoms");

      IRConst *size_const = gWordTy == Ity_I32 ?
          IRConst_U32(sizeofIRType(typeOfIRExpr(bb->tyenv, data_expr))) :
          IRConst_U64(sizeofIRType(typeOfIRExpr(bb->tyenv, data_expr)));

      IRExpr *size_expr = IRExpr_Const(size_const);
      IRDirty *ir_dirty = unsafeIRDirty_0_N(0,
          "check_mmap",
          check_mmap,
          mkIRExprVec_2(addr_expr, size_expr));
      addStmtToIRSB(new_bb, IRStmt_Dirty(ir_dirty));
    }
  }

  return new_bb;
}

static void ml_fini(Int exitcode)
{
  VG_(HT_destruct)(mmaps);
}

static inline UInt keys_needed(SizeT len)
{
  return (UInt) (len / VKI_PAGE_SIZE + (len % VKI_PAGE_SIZE ? 1 : 0));
}

static inline UWord current_key(Addr a, UInt _index)
{
  return (UWord) (a + _index * VKI_PAGE_SIZE);
}

static void new_nodes(Addr a, SizeT len)
{
  struct AddrNode *node;
  UInt i;

  for (i = 0; i < keys_needed(len); i++) {
    node = VG_(malloc)("ml-node", sizeof(struct AddrNode));
    tl_assert(NULL != node);
    node->key = current_key(a, i);
    UInt chunk = len - i * VKI_PAGE_SIZE;
    node->len = chunk > VKI_PAGE_SIZE ? VKI_PAGE_SIZE : chunk;
    VG_(HT_add_node)(mmaps, node);
  }
}

static void delete_nodes(Addr a, SizeT len)
{
  UInt i;

  for (i = 0; i < keys_needed(len); i++) {
    void *ret = VG_(HT_remove)(mmaps, current_key(a, i));
    tl_assert2(NULL != ret, "removing key failed - addr 0x%lx - %u",
        current_key(a, i), len);
    VG_(free)(ret);
  }
}

static void ml_track_new_mem_mmap(Addr a, SizeT len, Bool rr, Bool ww, Bool xx,
                                  ULong di_handle)
{
  tl_assert(VG_IS_PAGE_ALIGNED(a));
  VG_(message)(Vg_UserMsg, "mmap call - 0x%lx - %lu\n", a, len);

  new_nodes(a, len);
}

static void ml_track_die_mem_munmap(Addr a, SizeT len)
{
  tl_assert(VG_IS_PAGE_ALIGNED(a));
  delete_nodes(a, len);
}

static void ml_track_copy_mem_remap(Addr from, Addr to, SizeT len)
{
  tl_assert(VG_IS_PAGE_ALIGNED(from) && VG_IS_PAGE_ALIGNED(to));
  VG_(message)(Vg_UserMsg, "remap call - from 0x%lx to 0x%lx - %lu\n",
      from, to, len);
  delete_nodes(from, len);
  new_nodes(to, len);
}

static void ml_track_pre_mem_write(CorePart part, ThreadId tid, Char *s, Addr a,
                                   SizeT size)
{
  check_mmap(a, size);
}

static void ml_track_post_mem_write(CorePart part, ThreadId tid, Addr a,
                                    SizeT size)
{
}

static void ml_pre_clo_init(void)
{
  VG_(details_name)("MmapLogger");
  VG_(details_version)("pre-0.1");
  VG_(details_description)("logs access to mmaped files");
  VG_(details_copyright_author)(
    "Copyright (C) 2006, 2011, and GNU GPL'd, by Stephan Creutz.");
  VG_(details_bug_reports_to)("https://bitbucket.org/stephan_cr");

  VG_(basic_tool_funcs)(ml_post_clo_init, ml_instrument, ml_fini);

  VG_(track_new_mem_mmap)(ml_track_new_mem_mmap);
  VG_(track_die_mem_munmap)(ml_track_die_mem_munmap);

  VG_(track_pre_mem_write)(ml_track_pre_mem_write);
  VG_(track_post_mem_write)(ml_track_post_mem_write);
  VG_(track_copy_mem_remap)(ml_track_copy_mem_remap);
}

VG_DETERMINE_INTERFACE_VERSION(ml_pre_clo_init)
