isNaN(Math.hypot(NaN, 2)) &&
isNaN(Math.hypot(2, NaN)) &&
isNaN(Math.hypot(NaN, 2, 2)) &&
isNaN(Math.hypot(2, NaN, 2)) &&
isNaN(Math.hypot(2, 2, NaN)) &&
Math.hypot(0, -0) === 0 &&
(1 / Math.hypot(0, -0) === Infinity) &&
Math.hypot(0, 3) === 3 &&
Math.hypot(-0, -0) === 0 &&
(1 / Math.hypot(-0, -0) === Infinity) &&
Math.hypot(-0, 3) === 3 &&
Math.hypot(-0, -0, -0) === 0 &&
(1 / Math.hypot(-0, -0, -0) === Infinity) &&
Math.hypot(0, 0, 0) === 0 &&
(1 / Math.hypot(0, 0, 0) === Infinity) &&
Math.hypot(0, 0, -0) === 0 &&
(1 / Math.hypot(0, 0, -0) === Infinity) &&
Math.hypot(Infinity, 1) === Infinity &&
Math.hypot(-Infinity, 1) === Infinity &&
Math.hypot(1, Infinity) === Infinity &&
Math.hypot(1, -Infinity) === Infinity &&
Math.hypot(1, 1, Infinity) === Infinity &&
Math.hypot(1, 1, -Infinity) === Infinity &&
Math.hypot(NaN, NaN, Infinity) === Infinity &&
Math.hypot(NaN, Infinity, NaN) === Infinity &&
Math.hypot(Infinity, NaN, NaN) === Infinity;
