#pragma once

#define FETCH_UNUSED(x) (void)(x)

#ifdef __GNUC__
#define FETCH_MAYBE_UNUSED __attribute__((used))
#else
#define FETCH_MAYBE_UNUSED
#endif

#if defined(__clang__)
#define FETCH_THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define FETCH_THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#define FETCH_GUARDED_BY(x) FETCH_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define FETCH_PTR_GUARDED_BY(x) FETCH_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))
