isNaN(Math.expm1(NaN)) &&
Math.expm1(0) === 0 &&
(1 / Math.expm1(0) === Infinity) &&
Math.expm1(-0) === -0 &&
(1 / Math.expm1(-0) === -Infinity) &&
Math.expm1(Infinity) === Infinity &&
Math.expm1(-Infinity) === -1;
