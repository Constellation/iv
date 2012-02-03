isNaN(Math.tanh(NaN)) &&
Math.tanh(0) === 0 &&
(1 / Math.tanh(0) === Infinity) &&
Math.tanh(-0) === -0 &&
(1 / Math.tanh(-0) === -Infinity) &&
Math.tanh(Infinity) === 1 &&
Math.tanh(-Infinity) === -1;
