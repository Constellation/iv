describe("String", function() {
  it("reverse", function() {
    expect("TEST".reverse()).toBe("TSET");
    expect("日本語".reverse()).toBe("語本日");
    expect("日本+test語".reverse()).toBe("語tset+本日");
    expect("".reverse()).toBe("");
  });
});
