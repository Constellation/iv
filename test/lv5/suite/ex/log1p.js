isNaN(Math.log1p(NaN)) &&
isNaN(Math.log1p(-2)) &&
Math.log1p(-1) === -Infinity &&
Math.log1p(0) === 0 &&
(1 / Math.log1p(0) === Infinity) &&
Math.log1p(-0) === -0 &&
(1 / Math.log1p(-0) === -Infinity) &&
Math.log1p(Infinity) === Infinity;
