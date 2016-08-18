// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "processor.h"

#include <stdlib.h>

#include <acpica/acpi.h>
#include <mxio/dispatcher.h>

// Data associated with each message pipe handle
typedef struct {
    // The namespace node associated with this handle.  The
    // handle should be allowed to access ACPI resources further up
    // the namespace tree.
    ACPI_HANDLE ns_node;
} acpi_handle_ctx_t;

static mx_status_t dispatch(mx_handle_t h, void* _ctx, void* cookie) {
    //acpi_handle_ctx_t* ctx = _ctx;
    return NO_ERROR;
}

mx_status_t begin_processing(mx_handle_t root_handle) {
    acpi_handle_ctx_t* root_context = calloc(1, sizeof(acpi_handle_ctx_t));
    if (!root_context) {
        return ERR_NO_MEMORY;
    }

    mx_status_t status = ERR_BAD_STATE;
    ACPI_STATUS acpi_status = AcpiGetHandle(NULL,
                                            (char*)"\\_SB",
                                            &root_context->ns_node);
    if (acpi_status != AE_OK) {
        status = ERR_NOT_FOUND;
        goto fail;
    }

    mxio_dispatcher_t *dispatcher;
    status = mxio_dispatcher_create(&dispatcher, dispatch);
    if (status != NO_ERROR) {
        goto fail;
    }

    status = mxio_dispatcher_add(dispatcher, root_handle, root_context, NULL);
    if (status != NO_ERROR) {
        goto fail;
    }

    mxio_dispatcher_run(dispatcher);
    // mxio_dispatcher_run should not return
    return ERR_BAD_STATE;

fail:
    free(root_context);
    return status;
}
