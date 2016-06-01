node {

    stage 'Checkout'

        git url: 'https://github.com/marinatb/marinatb.git'
        sh 'git clean -fdx'
        sh 'git submodule init'
        sh 'git submodule update'


    stage 'Build'

        sh 'mkdir build'
        sh 'cd build ; cmake .. -G Ninja'
        sh 'cd build ; ninja'


    stage 'Test Setup'
        env.UUID = "mtb-${env.BRANCH_NAME}-${env.BUILD_NUMBER}"

        sh './builder pkg'

        parallel (

            'containerize-system': {
                sh './builder containerize-system'
            },

            'containerize-test': {
                sh './builder containerize test'
            },

            'docker-network': {
                sh './builder net || true'
            }

        )

        sh './builder launch-system'


    stage 'Integration Tests'

        sh '''env && timeout 15s docker run -i --hostname=test --net=${UUID}-tnet -v "${PWD}":/code --entrypoint /bin/bash --rm test:${UUID} <<EOT
/code/build/test/api/run_api_tests [api-blueprint]
EOT'''


    stage 'Cleanup'

        sh './builder terminate-system'
        sh 'docker network rm ${UUID}-tnet'
}

