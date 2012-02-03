isNaN(Math.trunc(NaN)) &&
Math.trunc(-0) === -0 &&
(1 / Math.trunc(-0) === -Infinity) &&
Math.trunc(0) === 0 &&
(1 / Math.trunc(0) === Infinity) &&
Math.trunc(Infinity) === Infinity &&
Math.trunc(-Infinity) === -Infinity;
