describe("Number", function() {
  it("isInteger", function() {
    expect(Number.isInteger(NaN)).toBe(false);
    expect(Number.isInteger(10)).toBe(true);
    expect(Number.isInteger(-20)).toBe(true);
    expect(Number.isInteger(222.222)).toBe(false);
    expect(Number.isInteger(-222.222)).toBe(false);
    expect(Number.isInteger(0)).toBe(true);
    expect(Number.isInteger(-0)).toBe(true);
    expect(Number.isInteger(Infinity)).toBe(true);
    expect(Number.isInteger(-Infinity)).toBe(true);
    expect(Number.isInteger("0")).toBe(false);
    expect(Number.isInteger(/test/)).toBe(false);
    expect(Number.isInteger({})).toBe(false);
    expect(Number.isInteger([])).toBe(false);
    expect(Number.isInteger(function() { })).toBe(false);
  });
});
