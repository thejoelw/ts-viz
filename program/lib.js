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

const normMean = (x, meanSize) => conv(windowSimple(meanSize), x);
const normStd = (x, meanSize, stdSize) =>
	sqrt(conv(windowSimple(stdSize), square(sub(x, normMean(x, meanSize)))));
const normVal = (x, meanSize, stdSize, eps = 1e-3) =>
	div(sub(x, normMean(x, meanSize)), add(normStd(x, meanSize, stdSize), eps));

const deltaEnergy = (x, deltaSize, smoothSize) =>
	sqrt(conv(windowSimple(smoothSize), square(conv(windowDelta(deltaSize), x))));

const correlation = (x, y, smoothSize) =>
	conv(windowSimple(smoothSize), mul(x, y));

const aboveMeanRatio = (x, meanSize, smoothSize) =>
	conv(windowSimple(smoothSize), gt(x, conv(windowSimple(meanSize), x)));

// From http://www.pgccphy.net/Linreg/linreg.pdf
// y = mx + b

const makeSs = (x, y, makeMeanSampler) => {
	const S_xx = sub(makeMeanSampler(square(x)), square(makeMeanSampler(x)));
	const S_xy = sub(
		makeMeanSampler(mul(x, y)),
		mul(makeMeanSampler(x), makeMeanSampler(y)),
	);
	const S_yy = sub(makeMeanSampler(square(y)), square(makeMeanSampler(y)));
	return [S_xx, S_xy, S_yy];
};

const makeRegM = (x, y, makeMeanSampler) => {
	const [S_xx, S_xy, S_yy] = makeSs(x, y, makeMeanSampler);
	return div(S_xy, S_xx);
};

const makeRegB = (x, y, makeMeanSampler) => {
	const [S_xx, S_xy, S_yy] = makeSs(x, y, makeMeanSampler);
	const top = sub(
		mul(makeMeanSampler(y), makeMeanSampler(square(x))),
		mul(makeMeanSampler(x), makeMeanSampler(mul(x, y))),
	);
	return div(top, S_xx);
};

const makeRegR = (x, y, makeMeanSampler) => {
	const [S_xx, S_xy, S_yy] = makeSs(x, y, makeMeanSampler);
	const top = S_xy;
	const bottom = sqrt(mul(S_xx, S_yy));
	return div(top, bottom);
};

const makeRegRsq = (x, y, makeMeanSampler) => {
	const [S_xx, S_xy, S_yy] = makeSs(x, y, makeMeanSampler);
	return div(square(S_xy), mul(S_xx, S_yy));
};

const makeRegResidualStd = (x, y, makeMeanSampler) => {
	const [S_xx, S_xy, S_yy] = makeSs(x, y, makeMeanSampler);
	return sqrt(sub(S_yy, div(square(S_xy), S_xx)));
};

module.exports = {
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
};
