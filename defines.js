module.exports = (variant) => ({
	ENABLE_GRAPHICS: !variant.match(/\b(?:headless|test)\b/),
	ENABLE_GUI: !variant.match(/\b(?:headless|test)\b/),

	// The maximum merge gap in bytes.
	// When updating flags, flags separated by a gap smaller than this value will be merged.
	GLBUFFER_MAX_MERGE_GAP: 64,

	GLBUFFER_FLUSH_EXPLICIT: 1,

	GLBUFFER_ASSERT_BINDINGS: '!defined(NDEBUG) && 1',

	GLVAO_ASSERT_BINDINGS: '!defined(NDEBUG) && 1',

	PRINT_TICK_ORDER: '!defined(NDEBUG) && 0',

	ENABLE_CHUNK_NAMES: '!defined(NDEBUG) && 0',

	ENABLE_CHUNK_MULTITHREADING: 0,

	INPUT_SERIES_ELEMENT_TYPE: 'double',

	CHUNK_SIZE_LOG2:
		{
			release: 10,
			'release-headless': 10,
			debug: 10,
			qtc: 8,
		}[variant] || variant.match(/\bcsl2-(\d+)\b/)[1],

	CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2: 4,
	CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2: 4,
	CONV_USE_FFT_ABOVE_SIZE_LOG2: 0,

	// Refer to https://docs.google.com/spreadsheets/d/1bx1zbFPLz8JTu8aoTONM20n3F2VC5FLHwV885wqKi4I/edit for information on how they work
	CONV_VARIANT: 'series::convvariant::ZpTs1',
	// TODO: Make other variants work
	// CONV_VARIANT: 'series::convvariant::ZpTs2',
	// CONV_VARIANT: 'series::convvariant::ZpKernel',
});
