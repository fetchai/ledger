python_binary(
    name="swarmeye",
    dependencies=[
        ':swarmeye_lib',
        ':misc',
        'src/resources/swarmeye/main/statics',
    ],
    shebang="#!/usr/bin/env python3",
    compatibility=">3.0",
    source='server.py',
)

python_library(
    name="swarmeye_lib",
    dependencies=[
        'src/python/utils',
        'src/python/monitoring',
        'src/python/bottle',
    ],
)

python_requirement_library(
    name='misc',
    requirements=[
        python_requirement('requests'),
    ]
)
