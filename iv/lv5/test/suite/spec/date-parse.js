describe("Date.parse", function() {
  it("1999-12-31T23:59:60.000 to be 946684800000", function() {
    expect(Date.parse('1999-12-31T23:59:60.000')).toBe(946684800000);
  });
});
