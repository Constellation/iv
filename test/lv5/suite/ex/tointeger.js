Number.toInteger(NaN) === 0 &&
Number.toInteger(10) === 10 &&
Number.toInteger(-20) === -20 &&
Number.toInteger(222.222) === 222 &&
Number.toInteger(-222.222) === -222 &&
Number.toInteger(0) === 0 &&
(1 / Number.toInteger(0) === Infinity) &&
Number.toInteger(-0) === -0 &&
(1 / Number.toInteger(-0) === -Infinity) &&
Number.toInteger(Infinity) === Infinity &&
Number.toInteger(-Infinity) === -Infinity &&
Number.toInteger("0") === 0 &&
(1 / Number.toInteger("0") === Infinity) &&
Number.toInteger("-0") === 0 &&
(1 / Number.toInteger("-0") === -Infinity) &&
Number.toInteger("t") === 0 &&
(1 / Number.toInteger("t") === Infinity) &&
Number.toInteger(/test/) === 0 &&
(1 / Number.toInteger(/test/) === Infinity) &&
Number.toInteger({}) === 0 &&
(1 / Number.toInteger({}) === Infinity) &&
Number.toInteger([]) === 0 &&
(1 / Number.toInteger([]) === Infinity) &&
Number.toInteger(function() { }) === 0 &&
(1 / Number.toInteger(function() { }) === Infinity);
