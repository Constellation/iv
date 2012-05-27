describe("Math", function() {
  it('log10', function() {
    expect(isNaN(Math.log10(NaN))).toBe(true);
    expect(isNaN(Math.log10(-1))).toBe(true);
    expect(Math.log10(0)).toBe(-Infinity);
    expect(Math.log10(-0)).toBe(-Infinity);
    expect(Math.log10(1)).toBe(0);
    expect(1 / Math.log10(1)).toBe(Infinity);
    expect(Math.log10(Infinity)).toBe(Infinity);
  });
});
