describe("Math", function() {
  it("expm1", function() {
    expect(isNaN(Math.expm1(NaN))).toBe(true);
    expect(Math.expm1(0)).toBe(0);
    expect(1 / Math.expm1(0)).toBe(Infinity);
    expect(Math.expm1(-0)).toBe(-0)
    expect(1 / Math.expm1(-0)).toBe(-Infinity);
    expect(Math.expm1(Infinity)).toBe(Infinity)
    expect(Math.expm1(-Infinity)).toBe(-1);
  });
});
