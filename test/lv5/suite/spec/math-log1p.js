describe("Math", function() {
  it('log1p', function() {
    expect(isNaN(Math.log1p(NaN))).toBe(true);
    expect(isNaN(Math.log1p(-2))).toBe(true);
    expect(Math.log1p(-1)).toBe(-Infinity);
    expect(Math.log1p(0)).toBe(0);
    expect(1 / Math.log1p(0)).toBe(Infinity);
    expect(Math.log1p(-0)).toBe(-0);
    expect(1 / Math.log1p(-0)).toBe(-Infinity);
    expect(Math.log1p(Infinity)).toBe(Infinity);
  });
});
