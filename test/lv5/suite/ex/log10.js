isNaN(Math.log10(NaN)) &&
isNaN(Math.log10(-1)) &&
Math.log10(0) === -Infinity &&
Math.log10(-0) === -Infinity &&
Math.log10(1) === 0 &&
(1 / Math.log10(1) === Infinity) &&
Math.log10(Infinity) === Infinity;
