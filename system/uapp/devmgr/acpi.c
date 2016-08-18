// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "acpi.h"

#include <stdio.h>

#include <launchpad/launchpad.h>

#include <magenta/processargs.h>
#include <magenta/syscalls.h>

#include "vfs.h"

static mx_handle_t acpi_root;

mx_status_t devmgr_launch_acpisvc(void) {
    const char* binname = "/boot/bin/acpisvc";

    const char* args[3] = {
        binname,
    };

    mx_handle_t pcie_ready = mx_event_create(0);
    if (pcie_ready < 0) {
        return pcie_ready;
    }
    mx_handle_t remote_pcie_ready =
            mx_handle_duplicate(pcie_ready, MX_RIGHT_SAME_RIGHTS);
    if (remote_pcie_ready < 0) {
        mx_handle_close(pcie_ready);
        return remote_pcie_ready;
    }

    mx_handle_t pipe[2];
    mx_status_t status = mx_message_pipe_create(pipe, 0);
    if (status != NO_ERROR) {
        mx_handle_close(remote_pcie_ready);
        mx_handle_close(pcie_ready);
        return status;
    }

    mx_handle_t hnd[3];
    uint32_t ids[3];
    // TODO(teisenbe): Does acpisvc need FS access?
    ids[0] = MX_HND_TYPE_MXIO_ROOT;
    hnd[0] = vfs_create_root_handle();
    ids[1] = MX_HND_TYPE_USER1;
    hnd[1] = pipe[1];
    ids[2] = MX_HND_TYPE_USER2;
    hnd[2] = mx_handle_duplicate(pcie_ready, MX_RIGHT_SAME_RIGHTS);
    printf("devmgr: launch acpisvc\n");
    mx_handle_t proc = launchpad_launch("acpisvc", 1, args, NULL, 3, hnd, ids);
    if (proc < 0) {
        printf("devmgr: launch failed: %d\n", proc);
        mx_handle_close(pcie_ready);
        mx_handle_close(remote_pcie_ready);
        mx_handle_close(pipe[0]);
        mx_handle_close(pipe[1]);
        return proc;
    }

    mx_handle_close(proc);
    acpi_root = pipe[0];

    // Wait for PCIe to be initialized
    while (1) {
        mx_signals_state_t state;
        mx_status_t status = mx_handle_wait_one(pcie_ready,
                                                MX_SIGNAL_SIGNAL0,
                                                MX_TIME_INFINITE,
                                                &state);
        if (status != NO_ERROR) {
            continue;
        }
        if (state.satisfied == MX_SIGNAL_SIGNAL0) {
            break;
        }
    }

    mx_handle_close(pcie_ready);

    return NO_ERROR;
}
