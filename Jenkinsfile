node {

    stage 'Checkout'

        git url: 'https://github.com/marinatb/marinatb.git'
        sh 'git clean -fdx'
        sh 'git submodule init'
        sh 'git submodule update'

    stage 'Environment Setup'

        echo 'Do something'

    stage 'Build'

        sh 'mkdir build'
        sh 'cmake . -G Ninja'
        sh 'ninja'

    stage 'Integration Test'

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


    stage 'Cleanup'

        sh './builder terminate-system'
        sh 'docker network rm tnet'
}

