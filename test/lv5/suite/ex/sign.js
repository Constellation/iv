isNaN(Math.sign(NaN)) &&
Math.sign(-0) === -0 &&
(1 / Math.sign(-0) === -Infinity) &&
Math.sign(0) === 0 &&
(1 / Math.sign(0) === Infinity) &&
Math.sign(-20) === -1 &&
Math.sign(20) === 1;
