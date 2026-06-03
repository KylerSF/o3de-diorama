{
    "Source" : "DioramaSprite.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : false, "WriteMask" : "Zero", "CompareFunc" : "Always" }
    },

    "RasterState" : { "CullMode" : "None" },

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
