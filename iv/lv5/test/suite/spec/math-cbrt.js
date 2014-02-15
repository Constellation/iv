describe("Math", function() {
  it('cbrt', function() {
    expect(isNaN(Math.cbrt(NaN))).toBe(true);
    expect(Math.cbrt(0)).toBe(0);
    expect(1 / Math.cbrt(0)).toBe(Infinity);
    expect(Math.cbrt(-0)).toBe(0);
    expect(1 / Math.cbrt(-0)).toBe(-Infinity);
    expect(Math.cbrt(Infinity)).toBe(Infinity);
    expect(Math.cbrt(-Infinity)).toBe(-Infinity);
  });
});
