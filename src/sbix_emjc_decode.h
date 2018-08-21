//
//  sbix_emjc_decode.h
//  sbix/emjc decoder
//
//  Created by cc4966 on 2018/08/19.
//

#ifndef sbix_emjc_decode_h
#define sbix_emjc_decode_h

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#  define __attribute__(X)
#  pragma warning(disable : 4068)
#endif

#if defined(EMJC_DLL)
#  if defined(_WIN32) || defined(__CYGWIN__)
#    if defined(EMJC_DLL_EXPORTS)
#      define EMJC_API __declspec(dllexport)
#    else
#      define EMJC_API __declspec(dllimport)
#    endif
#  endif
#endif

#if !defined(EMJC_API)
#  if __GNUC__ >= 4
#    define EMJC_API __attribute__((visibility("default")))
#  else
#    define EMJC_API
#  endif
#endif

EMJC_API uint16_t emjc_width(const uint8_t *__restrict src_buffer,
                             size_t src_size);

EMJC_API uint16_t emjc_height(const uint8_t *__restrict src_buffer,
                              size_t src_size);

EMJC_API size_t emjc_decode_buffer_size(const uint8_t *__restrict src_buffer,
                                        size_t src_size);

EMJC_API int emjc_decode_buffer(uint8_t *__restrict dst_buffer,
                                const uint8_t *__restrict src_buffer,
                                size_t src_size,
                                void *__restrict decode_buffer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* sbix_emjc_decode_h */

