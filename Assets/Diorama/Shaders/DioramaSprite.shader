{
    "Source" : "DioramaSprite.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : true, "WriteMask" : "Zero", "CompareFunc" : "GreaterEqual" }
    },

    "GlobalTargetBlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add"
    },

    "ProgramSettings" : {
        "EntryPoints" : [
            { "name" : "MainVS", "type" : "Vertex" },
            { "name" : "MainPS", "type" : "Fragment" }
        ]
    },

    "DrawList" : "transparent"
}
