/**
 * @file
 * @brief Embedded member to owning structure conversion.
 */

#pragma once
#include "minilib/stddef.h"

/**
 * @brief Get the structure owning an embedded member.
 *
 * @param PTR Pointer to the embedded member.
 * @param TYPE Type of the owning structure.
 * @param MEMBER Name of the member inside @p TYPE.
 */
#define container_of(PTR, TYPE, MEMBER) \
	((TYPE *)((char *)(PTR) - offset_of(TYPE, MEMBER)))
