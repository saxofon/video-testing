#!/bin/env python3

# SPDX-License-Identifier: GPL-3.0
#
# Author : Per Hallsmark <per@hallsmark.se>
#

import os

def collect_files_from_directories(directories):
    """Recursively collect all files from the given directories."""
    input_filenames = []
    for directory in directories:
        for root, dirs, files in os.walk(directory):
            for file in files:
                input_filenames.append(os.path.join(root, file))
    return input_filenames

def generate_c_and_header_files(input_directories, output_resources_c, output_resources_h):
    # Collect all input files from the directories
    input_filenames = collect_files_from_directories(input_directories)
    
    with open(output_resources_c, 'w') as resources_c, open(output_resources_h, 'w') as resources_h:
        # Write the header of the C source file

        # Write the header file guards
        resources_h.write('#ifndef __BUILTIN_RESOURCES_H__\n')
        resources_h.write('#define __BUILTIN_RESOURCES_H__\n\n')

        for input_filename in input_filenames:
            # Read the input file
            with open(input_filename, 'rb') as infile:
                data = infile.read()
            
            # Create a valid C array name from the input filename
            relative_path = os.path.relpath(input_filename, os.path.commonpath(input_directories))
            array_name = relative_path.replace(os.path.sep, '_').replace('-', '_').replace(' ', '_')
            array_name = os.path.splitext(array_name)[0]

            # Define the array in the C source file
            resources_c.write(f'const unsigned char {array_name}[] = {{\n')
            for i, byte in enumerate(data):
                if i % 16 == 0:
                    resources_c.write('\n')
                resources_c.write(f'0x{byte:02X}, ')
            resources_c.write('\n};\n\n')
            resources_c.write(f'const int {array_name}_size = sizeof({array_name}) / sizeof({array_name}[0]);\n')

            # Declare the array in the header file
            resources_h.write(f'extern const unsigned char {array_name};\n')
            resources_h.write(f'extern const int {array_name}_size;\n')

        # Close the header file guards
        resources_h.write('\n#endif\n')

# Usage example:
input_directories = ['src/resources/fonts', 'src/resources/icons']
generate_c_and_header_files(input_directories, 'build/builtin_resources.c', 'build/builtin_resources.h')
