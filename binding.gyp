{
    "targets": [
        {
            "target_name": "pdf_fill_form",
            "variables": {
                "myLibraries": "cairo poppler-qt5",
                "myIncludes": ""
            },            
            "sources": [
                "src/pdf-fill-form.cc",
                "src/NodePopplerAsync.cc",
                "src/NodePoppler.cc"
            ],
            'cflags!': [ '-fno-exceptions' ],
            'cflags_cc!': [ '-fno-exceptions' ],
            "cflags": [
                "<!@(pkg-config --cflags <(myLibraries) <(myIncludes))"
            ],
            "xcode_settings": {
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                "OTHER_CFLAGS": [
                    '-mmacosx-version-min=10.7',
                    '-std=c++11',
                    '-stdlib=libc++',                
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
