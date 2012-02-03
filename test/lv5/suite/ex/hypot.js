isNaN(Math.hypot(NaN, 2)) &&
isNaN(Math.hypot(2, NaN)) &&
Math.hypot(0, -0) === 0 &&
(1 / Math.hypot(0, -0) === Infinity) &&
Math.hypot(0, 3) === 3 &&
Math.hypot(-0, -0) === 0 &&
(1 / Math.hypot(-0, -0) === Infinity) &&
Math.hypot(-0, 3) === 3 &&
Math.hypot(Infinity, 1) === Infinity &&
Math.hypot(-Infinity, 1) === Infinity &&
Math.hypot(1, Infinity) === Infinity &&
Math.hypot(1, -Infinity) === Infinity;
