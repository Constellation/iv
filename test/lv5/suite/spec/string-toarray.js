describe("String", function() {
  it("toArray", function() {
    expect("TEST".toArray()).toEqual('TEST'.split(''));
    expect("日本語".toArray()).toEqual('日本語'.split(''));
    expect("日本+test語".toArray()).toEqual('日本+test語'.split(''));
    expect("TEST".toArray()).toEqual('TEST'.split(''));
    expect(("A" + "B" + "C" + "D" + "E" + "F" + "G" + "H").toArray()).toEqual("ABCDEFGH".split(''));
  });
});
