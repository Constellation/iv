isNaN(Math.log2(NaN)) &&
isNaN(Math.log2(-1)) &&
Math.log2(0) === -Infinity &&
Math.log2(-0) === -Infinity &&
Math.log2(1) === 0 &&
(1 / Math.log2(1) === Infinity) &&
Math.log2(Infinity) === Infinity;
