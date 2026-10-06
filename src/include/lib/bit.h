/**
 * @file
 * @brief Bit operations macros
 */

#pragma once

#define ALIGN_UP(addr, size)                               \
	((typeof(addr))(((uintptr_t)(addr) + (size) - 1) & \
			~((uintptr_t)(size) - 1)))
