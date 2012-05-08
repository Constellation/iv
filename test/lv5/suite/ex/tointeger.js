Number.toInt(NaN) === 0 &&
Number.toInt(10) === 10 &&
Number.toInt(-20) === -20 &&
Number.toInt(222.222) === 222 &&
Number.toInt(-222.222) === -222 &&
Number.toInt(0) === 0 &&
(1 / Number.toInt(0) === Infinity) &&
Number.toInt(-0) === -0 &&
(1 / Number.toInt(-0) === -Infinity) &&
Number.toInt(Infinity) === Infinity &&
Number.toInt(-Infinity) === -Infinity &&
Number.toInt("0") === 0 &&
(1 / Number.toInt("0") === Infinity) &&
Number.toInt("-0") === 0 &&
(1 / Number.toInt("-0") === -Infinity) &&
Number.toInt("t") === 0 &&
(1 / Number.toInt("t") === Infinity) &&
Number.toInt(/test/) === 0 &&
(1 / Number.toInt(/test/) === Infinity) &&
Number.toInt({}) === 0 &&
(1 / Number.toInt({}) === Infinity) &&
Number.toInt([]) === 0 &&
(1 / Number.toInt([]) === Infinity) &&
Number.toInt(function() { }) === 0 &&
(1 / Number.toInt(function() { }) === Infinity);
