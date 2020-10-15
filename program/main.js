const {
	input,
	f,
	d,
	sgn,
	abs,
	square,
	sqrt,
	exp,
	log,
	add,
	sub,
	mul,
	div,
	mod,
	lt,
	gt,
	min,
	max,
	shrink,
	windowRect,
	windowSimple,
	windowSmooth,
	windowDelta,
	conv,
	seq,
	plot,
} = require('./base');

const {
	normMean,
	normStd,
	normVal,
	deltaEnergy,
	correlation,
	aboveMeanRatio,
	makeSs,
	makeRegM,
	makeRegB,
	makeRegR,
	makeRegRsq,
	makeRegResidualStd,
} = require('./lib');

const main = () => {
	const r = d;

	const line = (x) => add(x, seq(r(0)));
	const raise = (x, m) => add(mul(x, r(m)), r(9.38));

	const mid = r(input('bin_com.btcusdt.log_mid'));
	// const mid_us = r(input('bin_us.btcusd.log_mid'));

	let ind = mid;
	ind = conv(
		windowDelta(r(100000), r(2)),
		sgn(conv(windowDelta(r(0.2), r(2)), ind)),
	);
	let ind2 = conv(windowDelta(r(100), r(2)), ind);
	// ind = add(ind, r(-0.01));

	// ind = mul(r(0.04), sgn(ind))
	// ind = ('shrink', ind, r(0.02))
	let weight = gt(ind, r(0.03));
	// weight = ('square', ind)
	const sum = add(mid, mul(r(2e-1), ind));
	const pred = div(
		add(mul(r(1e-1), mid), conv(windowSimple(r(20000)), mul(sum, weight))),
		add(r(1e-1), conv(windowSimple(r(20000)), weight)),
	);

	const size = 100000;

	return [
		plot(mid, 'mid com price'),
		plot(raise(ind, 0.1), 'ind'),
		// plot(
		// 	raise(
		// 		conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.bid_0'))),
		// 		1e-5,
		// 	),
		// 	'bid_0',
		// ),
		// plot(
		// 	raise(
		// 		conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.ask_0'))),
		// 		1e-5,
		// 	),
		// 	'ask_0',
		// ),
		plot(
			raise(
				conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.bid_1'))),
				1.2e-5,
			),
			'bid_1',
		),
		plot(
			raise(
				conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.ask_1'))),
				1.2e-5,
			),
			'ask_1',
		),
		plot(
			raise(
				gt(
					conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.bid_1'))),
					r(100),
				),
				1e-3,
			),
			'ind',
		),
		// plot(
		// 	raise(
		// 		conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.bid_2'))),
		// 		1.7e-5,
		// 	),
		// 	'bid_2',
		// ),
		// plot(
		// 	raise(
		// 		conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.ask_2'))),
		// 		1.7e-5,
		// 	),
		// 	'ask_2',
		// ),
		plot(
			raise(
				conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.bid_9'))),
				5e-3,
			),
			'bid_9',
		),
		plot(
			raise(
				conv(windowDelta(r(size), r(2)), r(input('bin_com.btcusdt.ask_9'))),
				5e-3,
			),
			'ask_9',
		),
		plot(
			raise(
				conv(
					windowDelta(r(size), r(2)),
					r(input('bin_com.btcusdt.bid_volume')),
				),
				2e-3,
			),
			'bid_volume',
		),
		plot(
			raise(
				conv(
					windowDelta(r(size), r(2)),
					r(input('bin_com.btcusdt.ask_volume')),
				),
				2e-3,
			),
			'ask_volume',
		),
		plot(line(r(9.38)), 'thresh'),
	];
};

console.log(JSON.stringify(main()));
