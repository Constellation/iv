describe("Math", function() {
  it("cosh", function() {
    expect(isNaN(Math.cosh(NaN))).toBe(true);
    expect(Math.cosh(0)).toBe(1);
    expect(Math.cosh(-0)).toBe(1);
    expect(Math.cosh(Infinity)).toBe(Infinity);
    expect(Math.cosh(-Infinity)).toBe(Infinity);
  });
});
