describe("Math", function() {
  it("acosh", function() {
    expect(isNaN(Math.acosh(NaN))).toBe(true);
    expect(isNaN(Math.acosh(0))).toBe(true);
    expect(Math.acosh(1)).toBe(0);
    expect(1 / Math.acosh(1)).toBe(Infinity);
    expect(Math.acosh(Infinity)).toBe(Infinity);
  });
});
