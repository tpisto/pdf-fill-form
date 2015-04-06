{
    "targets": [
        {
            "target_name": "pdf_fill_form",
            "variables": {
                "myNeededLibraries": 'cairo poppler-qt4'
            },            
            "sources": [
                "src/pdf-fill-form.cc",
                "src/NodePopplerAsync.cc",
                "src/NodePoppler.cc"
            ],
            "cflags": [
                "<!@(pkg-config --cflags <(myNeededLibraries))"
            ],
            "xcode_settings": {
                "OTHER_CFLAGS": [
                    "<!@(pkg-config --cflags <(myNeededLibraries))"
                ]
            },
            "libraries": [
                "<!@(pkg-config --libs <(myNeededLibraries))"
            ],
            "include_dirs" : [
               "<!(node -e \"require('nan')\")"
            ]
        }
    ]
}
