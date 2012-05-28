describe("Number", function() {
  it("isNaN", function() {
    expect(Number.isNaN(NaN)).toBe(true);
    expect(Number.isNaN(10)).toBe(false);
    expect(Number.isNaN(-20)).toBe(false);
    expect(Number.isNaN(222.222)).toBe(false);
    expect(Number.isNaN(-222.222)).toBe(false);
    expect(Number.isNaN(0)).toBe(false);
    expect(Number.isNaN(-0)).toBe(false);
    expect(Number.isNaN(Infinity)).toBe(false);
    expect(Number.isNaN(-Infinity)).toBe(false);
    expect(Number.isNaN("0")).toBe(false);
    expect(Number.isNaN(/test/)).toBe(false);
    expect(Number.isNaN({})).toBe(false);
    expect(Number.isNaN([])).toBe(false);
    expect(Number.isNaN(function() { })).toBe(false);
  });
});
