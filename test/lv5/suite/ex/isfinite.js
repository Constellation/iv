!Number.isFinite(NaN) &&
Number.isFinite(10) &&
Number.isFinite(-20) &&
Number.isFinite(222.222) &&
Number.isFinite(-222.222) &&
Number.isFinite(0) &&
Number.isFinite(-0) &&
!Number.isFinite(Infinity) &&
!Number.isFinite(-Infinity) &&
!Number.isFinite("0") &&
!Number.isFinite(/test/) &&
!Number.isFinite({}) &&
!Number.isFinite([]) &&
!Number.isFinite(function() { });
