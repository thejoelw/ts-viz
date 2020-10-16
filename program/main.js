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
	cumsum,
	cumprod,
	fwdFillZero,
	subDelta,
	divDelta,
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
	const res = [];

	const r = d;

	const line = (x) => add(x, seq(r(0)));
	const raise = (x, m) => add(mul(x, r(m)), r(9.38));

	const cumReturn = (market, invRatio) => {
		const keepRatio = 0.999 && 1;
		const gain = cumsum(mul(subDelta(market), invRatio));
		const fees = cumsum(mul(r(Math.log(keepRatio)), abs(subDelta(invRatio))));
		return add(gain, fees);
	};

	const size = 10000;

	const mid = r(input('bin_com.btcusdt.log_mid'));
	res.push(plot(mid, 'mid com price'));

	// 0.2 tick-based signals
	let ind = conv(
		windowDelta(r(10000), r(2)),
		sgn(conv(windowDelta(r(0.2), r(2)), mid)),
	);
	res.push(plot(raise(ind, 1e-1), 'ind'));

	const m = 2e3;
	let pd = fwdFillZero(
		div(log(conv(windowSimple(r(20000)), exp(mul(ind, r(m))))), r(m)),
		r(0.0),
	);
	res.push(plot(raise(pd, 1e-1), 'pd'));
	let nd = fwdFillZero(
		div(log(conv(windowSimple(r(20000)), exp(mul(ind, r(-m))))), r(m)),
		r(0.0),
	);
	res.push(plot(raise(nd, 1e-1), 'nd'));

	let sig = sgn(shrink(sub(pd, nd), r(0e-3)));
	sig = fwdFillZero(sig, r(-1.0));
	sig = add(mul(sig, r(0.5)), r(0.5));
	res.push(plot(raise(sig, 1e-3), 'sig'));
	// !!!!!! - 1% return over 2.5 days

	/*
	let ind2 = conv(windowDelta(r(100), r(2)), ind);
	res.push(plot(raise(ind2, 1), 'ind2'));
	let buy2 = gt(ind2, r(0.8e-3));
	// res.push(plot(raise(buy2, 3e-3), 'buy2'));
	let sell2 = lt(ind2, r(-0.23e-3));
	// res.push(plot(raise(sell2, -3e-3), 'sell2'));
	let sum2 = conv(windowDelta(r(100000), r(2)), sub(buy2, sell2));
	res.push(plot(raise(sum2, 1e-1), 'sum2'));
	let sig2 = sgn(shrink(sum2, r(0.8e-1)));
	sig2 = fwdFillZero(sig2, r(-1.0));
	sig2 = add(mul(sig2, r(0.5)), r(0.5));
	res.push(plot(raise(sig2, 3e-3), 'sig2'));
	*/

	// let ind3 = conv(windowSmooth(r(10000), r(2)), ind);
	// res.push(plot(raise(ind3, 1e-1), 'ind3'));

	// let sig = gt(ind, r(1e-2));
	// sig = add(sig, r(-0.01));

	// sig = mul(r(0.04), sgn(sig))
	// sig = ('shrink', sig, r(0.02))
	// let weight = gt(sig, r(0.03));
	// // weight = ('square', sig)
	// const sum = add(mid, mul(r(2e-1), sig));
	// const pred = div(
	// 	add(mul(r(1e-1), mid), conv(windowSimple(r(20000)), mul(sum, weight))),
	// 	add(r(1e-1), conv(windowSimple(r(20000)), weight)),
	// );

	// res.push(plot(raise(sig, 1e-3), 'sig'));
	res.push(plot(raise(cumReturn(mid, sig), 1), 'return'));
	res.push(plot(raise(line(r(0)), 0), 'thresh'));

	return res;
};

console.log(JSON.stringify(main()));
