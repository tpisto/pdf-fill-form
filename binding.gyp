{
    "targets": [
        {
            "target_name": "pdf_fill_form",
            "sources": [
                "src/pdf-fill-form.cc",
                "src/NodePopplerAsync.cc",
                "src/NodePoppler.cc"
            ],
            "libraries": [
                "<!@(pkg-config --libs cairo poppler-qt4)"
            ],
            "include_dirs" : [
               "<!(node -e \"require('nan')\")"
            ]
        }
    ]
}
