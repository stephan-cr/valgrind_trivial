/*
   Syscalltracer

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
#include "pub_tool_clreq.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_options.h"
#include "pub_tool_tooliface.h"

#include "function_tracing.h"
#include "syscall_table.h"

static Bool function_trace_option = False;
static Bool trace = True;

static void st_post_clo_init(void)
{
  if (function_trace_option) trace = False;
}

static IRSB* st_instrument(VgCallbackClosure* closure, IRSB* bb,
                           VexGuestLayout* layout, VexGuestExtents* vge,
                           IRType gWordTy, IRType hWordTy)
{
  return bb;
}

static void st_fini(Int exitcode)
{
}

static void st_pre_syscall(ThreadId tid, UInt syscallno, UWord *args,
                           UInt nArgs)
{
}

static void st_post_syscall(ThreadId tid, UInt syscallno, UWord *args,
                            UInt nArgs, SysRes res)
{
  if (!trace) return;

  if ((syscallno >= 0) && (syscallno < LAST_SYSCALL_NR)) {
    VG_(message)(Vg_UserMsg, "%s = %ld\n",
        syscall_map[syscallno], res._val);
  } else {
    VG_(message)(Vg_UserMsg, "%d = %ld\n", syscallno, res._val);
  }
}

static Bool st_handle_client_request(ThreadId tid, UWord *arg_block, UWord *ret)
{
  if (!VG_IS_TOOL_USERREQ('S', 'T', arg_block[0])) return False;
  if (!function_trace_option) return True;

  if (arg_block[0] == USERREQ_START_FUNCTION_TRACING) {
    VG_(message)(Vg_UserMsg, "start\n");
    trace = True;
    return True;
  } else if (arg_block[0] == USERREQ_STOP_FUNCTION_TRACING) {
    VG_(message)(Vg_UserMsg, "stop\n");
    trace  = False;
    return True;
  }

  return False;
}

static Bool st_process_cmd_line_option(Char *argv)
{
  return VG_BOOL_CLO(argv, "--function-trace", function_trace_option);
}

static void st_print_usage(void)
{
  VG_(printf)("--function-trace=[no]|yes\n");
}

static void st_print_debug_usage(void)
{
  VG_(printf)("--debug - not yet implemented\n");
}

static void st_pre_clo_init(void)
{
  VG_(details_name)            ("Syscalltracer");
  VG_(details_version)         (NULL);
  VG_(details_description)     ("traces syscalls");
  VG_(details_copyright_author)(
  "Copyright (C) 2006, 2011, and GNU GPL'd, by Stephan Creutz.");
  VG_(details_bug_reports_to)  (VG_BUGS_TO);

  VG_(basic_tool_funcs)(st_post_clo_init, st_instrument, st_fini);
  VG_(needs_syscall_wrapper)(st_pre_syscall, st_post_syscall);
  VG_(needs_client_requests)(st_handle_client_request);
  VG_(needs_command_line_options)(st_process_cmd_line_option,
      st_print_usage, st_print_debug_usage);
}

VG_DETERMINE_INTERFACE_VERSION(st_pre_clo_init)
