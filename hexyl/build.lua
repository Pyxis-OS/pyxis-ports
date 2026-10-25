return function(ctx)
  ctx.run({ "python3", ctx.recipe .. "/build.py", ctx.source, ctx.extra["terminal-size"],
    ctx.build, ctx.stage, assert(ctx.rust), ctx.sdk, ctx.cross_compile, ctx.recipe })
end
