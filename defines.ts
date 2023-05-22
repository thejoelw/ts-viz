export default (variant: string) => {
  const ENABLE_GRAPHICS = !variant.match(/\b(?:headless|test)\b/);

  const CHUNK_SIZE_LOG2 =
    (
      {
        // Not sure why it crashes when we set this to 20
        release: 16,
        'release-headless': 16,
        debug: 16,
        'debug-headless': 16,
        qtc: 16,
      } as { [key: string]: number }
    )[variant] || variant.match(/\bcsl2-(\d+)\b/)![1];

  return {
    // The --log-level flag can further raise this, but this eliminates logs below this level at compile-time
    SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_TRACE',
    // SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_DEBUG',
    // SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_INFO',
    // SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_WARN',
    // SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_ERROR',
    // SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_CRITICAL',
    // SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_OFF',

    ENABLE_GRAPHICS,
    ENABLE_GUI: ENABLE_GRAPHICS,

    RENDER_RESOLUTION_X: 1 << 12,

    // The maximum merge gap in bytes.
    // When updating flags, flags separated by a gap smaller than this value will be merged.
    GLBUFFER_MAX_MERGE_GAP: 64,

    GLBUFFER_FLUSH_EXPLICIT: 1,

    GLBUFFER_ASSERT_BINDINGS: '!defined(NDEBUG) && 1',

    GLVAO_ASSERT_BINDINGS: '!defined(NDEBUG) && 1',

    PRINT_TICK_ORDER: '!defined(NDEBUG) && 0', // Also requires SPDLOG_ACTIVE_LEVEL to be 'SPDLOG_LEVEL_TRACE' and --log-level trace
    ENABLE_CHUNK_NAMES: '!defined(NDEBUG) && 1', // Also requires SPDLOG_ACTIVE_LEVEL to be 'SPDLOG_LEVEL_TRACE' and --log-level trace
    ENABLE_NOTIFICATION_TRACING: '!defined(NDEBUG) && 0', // Requires ENABLE_CHUNK_NAMES; also requires SPDLOG_ACTIVE_LEVEL to be 'SPDLOG_LEVEL_TRACE' and --log-level trace

    ENABLE_CHUNK_MULTITHREADING: 0,
    ENABLE_FILEPOLLER_YIELDING:
      variant === 'qtc' || variant.match(/\btest\b/) ? 1 : 0, // Only used for tests; has a more predictable effect when multithreading is disabled

    FILEPOLLER_TICK_TIMEOUT_MS: ENABLE_GRAPHICS ? 10 : 1000,

    ENABLE_PMUOI_FLAG: 1, // --print-memory-usage-output-index

    INPUT_SERIES_ELEMENT_TYPE: 'double',

    CHUNK_SIZE_LOG2,
    CONV_CACHE_KERNEL_FFT_GTE_SIZE_LOG2: 10,
    // CONV_CACHE_TS_FFT_GTE_SIZE_LOG2: CHUNK_SIZE_LOG2 - 2,
    CONV_CACHE_TS_FFT_GTE_SIZE_LOG2: CHUNK_SIZE_LOG2,
    CONV_USE_FFT_GTE_SIZE_LOG2: 0, // TODO: Increase this

    ENABLE_CONV_MIN_COMPUTE_FLAG: ENABLE_GRAPHICS, // --conv-min-compute-log2

    // Refer to https://docs.google.com/spreadsheets/d/1bx1zbFPLz8JTu8aoTONM20n3F2VC5FLHwV885wqKi4I/edit for information on how they work
    CONV_VARIANT: 'series::convvariant::ZpTs1',
    // TODO: Make other variants work
    // CONV_VARIANT: 'series::convvariant::ZpTs2',
    // CONV_VARIANT: 'series::convvariant::ZpKernel',
    // We also need a variant that can compute the bigger FFTs earlier, so the output doesn't have to block waiting for them.

    // From http://www.fftw.org/fftw3_doc/Planner-Flags.html:
    //   FFTW_ESTIMATE specifies that, instead of actual measurements of different algorithms, a simple heuristic is used to pick a (probably sub-optimal) plan quickly. With this flag, the input/output arrays are not overwritten during planning.
    //   FFTW_MEASURE tells FFTW to find an optimized plan by actually computing several FFTs and measuring their execution time. Depending on your machine, this can take some time (often a few seconds). FFTW_MEASURE is the default planning option.
    //   FFTW_PATIENT is like FFTW_MEASURE, but considers a wider range of algorithms and often produces a “more optimal” plan (especially for large transforms), but at the expense of several times longer planning time (especially for large transforms).
    //   FFTW_EXHAUSTIVE is like FFTW_PATIENT, but considers an even wider range of algorithms, including many that we think are unlikely to be fast, to produce the most optimal plan but with a substantially increased planning time.

    // FFTWX_PLANNING_LEVEL: 'FFTW_ESTIMATE',
    FFTWX_PLANNING_LEVEL: 'FFTW_MEASURE',
    // FFTWX_PLANNING_LEVEL: 'FFTW_PATIENT',
    // FFTWX_PLANNING_LEVEL: 'FFTW_EXHAUSTIVE',

    GARBAGE_COLLECTOR_LEVELS: 1,
  };
};
