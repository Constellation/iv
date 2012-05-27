describe("Math", function() {
  it('log2', function() {
    expect(isNaN(Math.log2(NaN))).toBe(true);
    expect(isNaN(Math.log2(-1))).toBe(true);
    expect(Math.log2(0)).toBe(-Infinity);
    expect(Math.log2(-0)).toBe(-Infinity)
    expect(Math.log2(1)).toBe(0);
    expect(1 / Math.log2(1)).toBe(Infinity);
    expect(Math.log2(Infinity)).toBe(Infinity);
  });
});
