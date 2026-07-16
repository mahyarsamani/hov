    .arch armv8-a
    .text
    .align 2

    // -----------------------------------------------------------------
    // 64-bit Data Loaders (LDINDX)
    // -----------------------------------------------------------------

    // __hov_ldindx_x (64-bit data, 64-bit index)
    .global __hov_ldindx_x
    .type __hov_ldindx_x, %function
__hov_ldindx_x:
    mov x7, x1
    .inst 0xC9831046
    mov x0, x6
    mov x1, x7
    ret
    .size __hov_ldindx_x, .-__hov_ldindx_x

    // __hov_ldindx_w (64-bit data, 32-bit index)
    .global __hov_ldindx_w
    .type __hov_ldindx_w, %function
__hov_ldindx_w:
    uxtw x4, w4
    mov x7, x1
    .inst 0xC8831046
    mov x0, x6
    mov x1, x7
    ret
    .size __hov_ldindx_w, .-__hov_ldindx_w

    // -----------------------------------------------------------------
    // 32-bit Data Loaders (LDINDW)
    // -----------------------------------------------------------------

    // __hov_ldindw_x (32-bit data, 64-bit index)
    .global __hov_ldindw_x
    .type __hov_ldindw_x, %function
__hov_ldindw_x:
    mov x7, x1
    .inst 0x89831046
    mov x0, x6
    mov x1, x7
    ret
    .size __hov_ldindw_x, .-__hov_ldindw_x

    // __hov_ldindw_w (32-bit data, 32-bit index)
    .global __hov_ldindw_w
    .type __hov_ldindw_w, %function
__hov_ldindw_w:
    uxtw x4, w4
    mov x7, x1
    .inst 0x88831046
    mov w0, w6
    orr x0, x0, x7, lsl #32
    ret
    .size __hov_ldindw_w, .-__hov_ldindw_w

    // -----------------------------------------------------------------
    // 16-bit Data Loaders (LDINDH)
    // -----------------------------------------------------------------

    // __hov_ldindh_x (16-bit data, 64-bit index)
    .global __hov_ldindh_x
    .type __hov_ldindh_x, %function
__hov_ldindh_x:
    mov x7, x1
    .inst 0x49831046
    mov x0, x6
    mov x1, x7
    ret
    .size __hov_ldindh_x, .-__hov_ldindh_x

    // __hov_ldindh_w (16-bit data, 32-bit index)
    .global __hov_ldindh_w
    .type __hov_ldindh_w, %function
__hov_ldindh_w:
    uxtw x4, w4
    mov x7, x1
    .inst 0x48831046
    mov w0, w6
    orr x0, x0, x7, lsl #32
    ret
    .size __hov_ldindh_w, .-__hov_ldindh_w

    // -----------------------------------------------------------------
    // 8-bit Data Loaders (LDINDB)
    // -----------------------------------------------------------------

    // __hov_ldindb_x (8-bit data, 64-bit index)
    .global __hov_ldindb_x
    .type __hov_ldindb_x, %function
__hov_ldindb_x:
    mov x7, x1
    .inst 0x09831046
    mov x0, x6
    mov x1, x7
    ret
    .size __hov_ldindb_x, .-__hov_ldindb_x

    // __hov_ldindb_w (8-bit data, 32-bit index)
    .global __hov_ldindb_w
    .type __hov_ldindb_w, %function
__hov_ldindb_w:
    uxtw x4, w4
    mov x7, x1
    .inst 0x08831046
    mov w0, w6
    orr x0, x0, x7, lsl #32
    ret
    .size __hov_ldindb_w, .-__hov_ldindb_w

    // -----------------------------------------------------------------
    // Store Operations (STIND)
    // -----------------------------------------------------------------

    // __hov_stindx_x (64-bit data, 64-bit index)
    .global __hov_stindx_x
    .type __hov_stindx_x, %function
__hov_stindx_x:
    mov x6, x0
    mov x7, x1
    .inst 0xC9C31046
    ret
    .size __hov_stindx_x, .-__hov_stindx_x

    // __hov_stindx_w (64-bit data, 32-bit index)
    .global __hov_stindx_w
    .type __hov_stindx_w, %function
__hov_stindx_w:
    uxtw x4, w4
    mov x6, x0
    mov x7, x1
    .inst 0xC8C31046
    ret
    .size __hov_stindx_w, .-__hov_stindx_w

    // -----------------------------------------------------------------
    // 32-bit Data Stores (STINDW)
    // -----------------------------------------------------------------

    // __hov_stindw_x (32-bit data, 64-bit index)
    .global __hov_stindw_x
    .type __hov_stindw_x, %function
__hov_stindw_x:
    mov x6, x0
    mov x7, x1
    .inst 0x89C31046
    ret
    .size __hov_stindw_x, .-__hov_stindw_x

    // __hov_stindw_w (32-bit data, 32-bit index)
    .global __hov_stindw_w
    .type __hov_stindw_w, %function
__hov_stindw_w:
    uxtw x4, w4
    mov x6, x0
    mov x7, x1
    .inst 0x88C31046
    ret
    .size __hov_stindw_w, .-__hov_stindw_w

    // -----------------------------------------------------------------
    // 16-bit Data Stores (STINDH)
    // -----------------------------------------------------------------

    // __hov_stindh_x (16-bit data, 64-bit index)
    .global __hov_stindh_x
    .type __hov_stindh_x, %function
__hov_stindh_x:
    mov x6, x0
    mov x7, x1
    .inst 0x49C31046
    ret
    .size __hov_stindh_x, .-__hov_stindh_x

    // __hov_stindh_w (16-bit data, 32-bit index)
    .global __hov_stindh_w
    .type __hov_stindh_w, %function
__hov_stindh_w:
    uxtw x4, w4
    mov x6, x0
    mov x7, x1
    .inst 0x48C31046
    ret
    .size __hov_stindh_w, .-__hov_stindh_w

    // -----------------------------------------------------------------
    // 8-bit Data Stores (STINDB)
    // -----------------------------------------------------------------

    // __hov_stindb_x (8-bit data, 64-bit index)
    .global __hov_stindb_x
    .type __hov_stindb_x, %function
__hov_stindb_x:
    mov x6, x0
    mov x7, x1
    .inst 0x09C31046
    ret
    .size __hov_stindb_x, .-__hov_stindb_x

    // __hov_stindb_w (8-bit data, 32-bit index)
    .global __hov_stindb_w
    .type __hov_stindb_w, %function
__hov_stindb_w:
    uxtw x4, w4
    mov x6, x0
    mov x7, x1
    .inst 0x08C31046
    ret
    .size __hov_stindb_w, .-__hov_stindb_w
