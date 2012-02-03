isNaN(Math.sinh(NaN)) &&
Math.sinh(0) === 0 &&
(1 / Math.sinh(0) === Infinity) &&
Math.sinh(-0) === -0 &&
(1 / Math.sinh(-0) === -Infinity) &&
Math.sinh(Infinity) === Infinity &&
Math.sinh(-Infinity) === -Infinity;
