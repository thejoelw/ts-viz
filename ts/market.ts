import { Node } from './types.ts';
import {
  abs,
  add,
  cond,
  cumSum,
  delay,
  eq,
  floor,
  fwdFillZero,
  gt,
  gte,
  i64,
  isNan,
  lt,
  max,
  min,
  monotonify,
  mul,
  not,
  plot,
  scanIf,
  sub,
  subDelta,
} from './base.ts';
import { r, raise } from './config.ts';
import { sum } from './vec.ts';

export const sim = (market: Node, cmd: Node, tradeFee = 0.00075) => {
  const keepRatio = 1 - tradeFee;
  const invRatio = add(mul(cmd, r(0.5)), r(0.5));
  const trade = abs(subDelta(invRatio));
  const gain = cumSum(mul(subDelta(market), invRatio));
  const fees = cumSum(mul(r(Math.log(keepRatio)), trade));
  return {
    market: cumSum(subDelta(market)),
    value: add(gain, fees),
    gain,
    fees,
    trades: cumSum(trade),
  };
};

export const plotReturn = (mid: Node, sigs: Node[]) => {
  const mode = 'default_sold';
  sigs = sigs.filter(Boolean);
  const enterSig = gt(subDelta(gt(sum(sigs), r(sigs.length * 0.999))), r(0.0));

  const exitThresh = fwdFillZero(
    mul(
      add(mid, r({ default_sold: 4e-3, default_bought: -4e-3 }[mode])),
      enterSig,
    ),
  );

  const exitSig = { default_sold: gt, default_bought: lt }[mode](
    mid,
    exitThresh,
  );

  const sig = fwdFillZero(sub(enterSig, exitSig));

  const retrn = sim(
    mid,
    add(mul(sig, r({ default_sold: 0.5, default_bought: -0.5 }[mode])), r(0.5)),
  ).value;

  return [
    plot('exit thresh', exitThresh),
    plot('sig', raise(sig, 1e-3)),
    plot('return', raise(retrn, 1)),
  ];
};

export const simMaker = (
  timeMs: Node,
  targBid: Node,
  targAsk: Node,
  xchBid: Node,
  xchAsk: Node,
  oneWayDelayTicks = 50,
) => {
  // MUST ensure that targBid < targAsk

  const orderIntervalTimeMs = 100;
  // const makerDistance = 2.5e-7; // Approx $0.01 at 40,000 USD/BTC
  const makerDistance = 1e-6; // Approx $0.04 at 40,000 USD/BTC

  const pgNan = (from: Node, to: Node) => cond(isNan(from), r(NaN), to);

  const xchBidView = delay(xchBid, i64(oneWayDelayTicks));
  const xchAskView = delay(xchAsk, i64(oneWayDelayTicks));

  // Try to place a maker order
  const sendBid = pgNan(
    targBid,
    min(targBid, sub(xchAskView, r(makerDistance))),
  );
  const sendAsk = pgNan(
    targAsk,
    max(targAsk, add(xchBidView, r(makerDistance))),
  );

  const didPlaceBid = lt(delay(sendBid, i64(oneWayDelayTicks)), xchAskView);
  const didPlaceAsk = gt(delay(sendAsk, i64(oneWayDelayTicks)), xchBidView);

  const changes = subDelta(
    floor(mul(monotonify(timeMs), r(1 / orderIntervalTimeMs))),
  );
  const activeBid = scanIf(
    not(
      add(
        // mul(changes, didPlaceBid),
        didPlaceBid,
        isNan(delay(sendBid, i64(oneWayDelayTicks))),
      ),
    ),
    delay(sendBid, i64(oneWayDelayTicks)),
    r(NaN),
  );

  const activeAsk = scanIf(
    not(
      add(
        // mul(changes, didPlaceAsk),
        didPlaceAsk,
        isNan(delay(sendAsk, i64(oneWayDelayTicks))),
      ),
    ),
    delay(sendAsk, i64(oneWayDelayTicks)),
    r(NaN),
  );

  const buys = gt(activeBid, xchAsk);
  const sells = lt(activeAsk, xchBid);
  const pos = fwdFillZero(sub(buys, sells));

  // First operation (from zero) must be a buy
  const buyValues = cond(gte(subDelta(pos), r(1)), activeBid, r(0));
  const sellValues = cond(eq(subDelta(pos), r(-2)), activeAsk, r(0));
  const value = add(
    cumSum(sub(sellValues, buyValues)),
    cond(gt(pos, r(0)), xchAsk, r(0)),
  );

  return { sendBid, sendAsk, activeBid, activeAsk, pos, value };
};
