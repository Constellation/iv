describe("String", function() {
  it("fromCodePoint", function() {
    expect(String.fromCodePoint(0x20)).toBe(' ');
    expect(String.fromCodePoint(0x20bb7)).toBe('\ud842\udfb7');
  });

  it("fromCodePoint floating", function() {
    expect(function () {
      String.fromCodePoint(0.2);
    }).toThrow();
  });

  it("fromCodePoint too big", function() {
    expect(function () {
      String.fromCodePoint(0xFFFFFFFFF);
    }).toThrow();
  });

  it("fromCodePoint negative", function() {
    expect(function () {
      String.fromCodePoint(-20);
    }).toThrow();
  });
});
