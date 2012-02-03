!Number.isInteger(NaN) &&
Number.isInteger(10) &&
Number.isInteger(-20) &&
!Number.isInteger(222.222) &&
!Number.isInteger(-222.222) &&
Number.isInteger(0) &&
Number.isInteger(-0) &&
Number.isInteger(Infinity) &&
Number.isInteger(-Infinity) &&
!Number.isInteger("0") &&
!Number.isInteger(/test/) &&
!Number.isInteger({}) &&
!Number.isInteger([]) &&
!Number.isInteger(function() { });
