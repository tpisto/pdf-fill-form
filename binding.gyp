{
    "targets": [
        {
            "target_name": "pdf_fill_form",
            "variables": {
                "myLibraries": "cairo poppler-qt4",
                "myIncludes": "QtCore"
            },            
            "sources": [
                "src/pdf-fill-form.cc",
                "src/NodePopplerAsync.cc",
                "src/NodePoppler.cc"
            ],
            "cflags": [
                "-fexceptions",
                "<!@(pkg-config --cflags <(myLibraries) <(myIncludes))"
            ],
            "xcode_settings": {
                "OTHER_CFLAGS": [
                    "-fexceptions",
                    "<!@(pkg-config --cflags <(myLibraries) <(myIncludes))"
                ]
            },
            "libraries": [
                "<!@(pkg-config --libs <(myLibraries))"
            ],
            "include_dirs" : [
               "<!(node -e \"require('nan')\")"
            ]
        }
    ]
}
