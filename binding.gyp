{
    "variables": {
        "major_version": "<!(node -pe 'v=process.versions.node.split(\".\"); v[0];')",
        "minor_version": "<!(node -pe 'v=process.versions.node.split(\".\"); v[1];')",
        "micro_version": "<!(node -pe 'v=process.versions.node.split(\".\"); v[2];')"
    },
    "targets": [
        {
            "target_name": "piston-poppler",
            "sources": [
                "src/PistonPoppler.cc",
                "src/NodePoppler.cc"
            ],
            "libraries": [
                "<!@(pkg-config --libs poppler-qt4)"
            ],
            "cflags": [
                "<!@(pkg-config --cflags poppler)"
            ],
            "include_dirs" : [
               "<!(node -e \"require('nan')\")"
            ],
            "defines": [
                "NODE_VERSION_MAJOR=<(major_version)",
                "NODE_VERSION_MINOR=<(minor_version)",
                "NODE_VERSION_MICRO=<(micro_version)"
            ],
            "xcode_settings": {
                "OTHER_CFLAGS": [
                    "<!@(pkg-config --cflags poppler)"
                ]
            }
        }
    ]
}
