Math.hypot2() === 0 &&
(1 / Math.hypot2()) === Infinity &&
isNaN(Math.hypot2(NaN, 2)) &&
isNaN(Math.hypot2(2, NaN)) &&
isNaN(Math.hypot2(NaN, 2, 2)) &&
isNaN(Math.hypot2(2, NaN, 2)) &&
isNaN(Math.hypot2(2, 2, NaN)) &&
Math.hypot2(0, -0) === 0 &&
(1 / Math.hypot2(0, -0) === Infinity) &&
Math.hypot2(0, 3) === 9 &&
Math.hypot2(-0, -0) === 0 &&
(1 / Math.hypot2(-0, -0) === Infinity) &&
Math.hypot2(-0, 3) === 9 &&
Math.hypot2(-0, -0, -0) === 0 &&
(1 / Math.hypot2(-0, -0, -0) === Infinity) &&
Math.hypot2(0, 0, 0) === 0 &&
(1 / Math.hypot2(0, 0, 0) === Infinity) &&
Math.hypot2(0, 0, -0) === 0 &&
(1 / Math.hypot2(0, 0, -0) === Infinity) &&
Math.hypot2(Infinity, 1) === Infinity &&
Math.hypot2(-Infinity, 1) === Infinity &&
Math.hypot2(1, Infinity) === Infinity &&
Math.hypot2(1, -Infinity) === Infinity &&
Math.hypot2(1, 1, Infinity) === Infinity &&
Math.hypot2(1, 1, -Infinity) === Infinity &&
Math.hypot2(NaN, NaN, Infinity) === Infinity &&
Math.hypot2(NaN, Infinity, NaN) === Infinity &&
Math.hypot2(Infinity, NaN, NaN) === Infinity;
