isNaN(Math.atanh(NaN)) &&
isNaN(Math.atanh(-2)) &&
isNaN(Math.atanh(2)) &&
Math.atanh(-1) === -Infinity &&
Math.atanh(1) === Infinity &&
Math.tanh(0) === 0 &&
(1 / Math.tanh(0) === Infinity) &&
Math.tanh(-0) === -0 &&
(1 / Math.tanh(-0) === -Infinity);
