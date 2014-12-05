/*
 * Copyright (c) 2011 Anil Madhavapeddy <anil@recoil.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <mini-os/os.h>
#include <mini-os/sched.h>

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/callback.h>

void _exit(int);
int errno;
static char *argv[] = { "mirage", NULL };
static unsigned long irqflags;

CAMLprim value
caml_block_domain(value v_until)
{
  CAMLparam1(v_until);
  block_domain((s_time_t)(Int64_val(v_until)));
  CAMLreturn(Val_unit);
}

void app_main_thread(void *unused)
{
  value *v_main;
  int caml_completed = 0;
  printk("xencaml: app_main_thread\n");
  local_irq_save(irqflags);
  caml_startup(argv);
  _exit(0);
}

void start_kernel(void)
{
  printk("Mirage: start_kernel\n");

  /* Set up events. */
  init_events();

  /* Enable event delivery. This is disabled at start of day. */
  local_irq_enable();

  setup_xen_features();

  /* Init memory management.
   * Needed for malloc. */
  init_mm();

  /* Init time and timers. Needed for block_domain. */
  init_time();

  /* Init the console driver.
   * We probably do need this if we want printk to send notifications correctly. */
  init_console();

  /* Init grant tables. */
  init_gnttab();

#if 1
    /* Call our main function directly, without using Mini-OS threads. */
  app_main_thread(NULL);
#else
  /* Init scheduler. */
  /* Needed if you want to use create_thread, but we can get away without it. */
  init_sched();

  /* Init XenBus */
  /* Using Mini-OS's XenBus support requires threads. */
  init_xenbus();

  /* Respond to "xl shutdown". Requires XenBus. */
  create_thread("shutdown", shutdown_thread, NULL);

  create_thread("ocaml", app_main_thread, NULL);

  /* Everything initialised, start idle thread */
  run_idle_thread();
#endif
}

void _exit(int ret)
{
  printk("main returned %d\n", ret);
  stop_kernel();
  if (!ret) {
    /* No problem, just shutdown.  */
    struct sched_shutdown sched_shutdown = { .reason = SHUTDOWN_poweroff };
    HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
  }
  do_exit();
}
